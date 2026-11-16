/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: manualva <manualva@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/26 15:57:49 by manualva          #+#    #+#             */
/*   Updated: 2026/11/16 18:37:29 by manualva         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"
#include <signal.h>

/*Initializes the server*/
Server::Server(int port, const std::string &password): _listenFd(-1), _port(port), _password(password), _commandHandler(*this){setupSocket();}

/*
Destroys the server

Closes every file descriptor stored in _pollFds and destroys every dynamically allocated client and channel
Finally, clears the containers
*/
Server::~Server()
{
	for (std::vector<struct pollfd>::iterator it = _pollFds.begin(); it != _pollFds.end(); ++it)
		close(it->fd);

	for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		delete it->second;

	for (std::map<std::string, Channel *>::iterator it = _channels.begin(); it != _channels.end(); ++it)
		delete it->second;

	_channels.clear();
	_clients.clear();
	_pollFds.clear();
}

//Does nothing, just there to interrupt poll()
static void handleSigint(int){}

/*
Creates the listening socket and prepares it to accept connections

Ignores sigpipe to prevent it from terminating the server
Configures custom signal handler for ctrl-C
Creates an IPv4 TCP socket
Enables adress reuse to quickly reactivate it after being destroyed
Makes the socket non-blocking so functions like accept() don't wait forever
	(returns immediately if no connection is waiting)
Configures the server address
	AF_INET: IPv4.
	INADDR_ANY: Accept connections through available interfaces
	_port: Sets up the port given (I like 1234)
	htons() and htonl(): Convert values into network byte order
Binds the socket with the IP address and port
Marks the socket as listening
	SOMAXCONN specifies the maximum connection backlog supported
Registers the listening socket with your polling system.
	POLLIN tells poll() when the socket is ready to acceptconnections.
*/
void Server::setupSocket()
{
	signal(SIGPIPE, SIG_IGN);

	struct sigaction sa;
	std::memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handleSigint;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	if (sigaction(SIGINT, &sa, NULL) < 0)
	throw std::runtime_error("sigaction() failed");

	_listenFd = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenFd < 0)
		throw std::runtime_error("socket() failed");

	int opt = 1;
	if (setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		close(_listenFd);
		_listenFd = -1;
		throw std::runtime_error("setsockopt() failed");
	}

	int flags = fcntl(_listenFd, F_GETFL, 0);
	if (flags < 0 || fcntl(_listenFd, F_SETFL, flags | O_NONBLOCK) < 0)
	{
		close(_listenFd);
		_listenFd = -1;
		throw std::runtime_error("fcntl() failed");
	}

	struct sockaddr_in address;
	std::memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(static_cast<unsigned short>(_port));

	if (bind(_listenFd, reinterpret_cast<struct sockaddr *>(&address), sizeof(address)) < 0)
	{
		close(_listenFd);
		_listenFd = -1;
		throw std::runtime_error("bind() failed");
	}

	if (listen(_listenFd, SOMAXCONN) < 0)
	{
		close(_listenFd);
		_listenFd = -1;
		throw std::runtime_error("listen() failed");
	}

	struct pollfd pfd;
	pfd.fd = _listenFd;
	pfd.events = POLLIN;
	pfd.revents = 0;
	_pollFds.push_back(pfd);

	std::cout << "Server listening on port " << _port << std::endl;
}

/*
Accepts a new client connection and connects it to the server

Retrieves connection from the listening socket
	returns if no connection is waiting
fcntl() sets the newly socket to non-blocking mode
Creates a new pollfd() for that client, descriptor is added to _pollFds
	_pollFds tracks sockets that poll() should monitor.
Creates and stores a new client in _clients
*/
void Server::acceptClient()
{
	struct sockaddr_in clientAddress;
	socklen_t addressLength = sizeof(clientAddress);

	int clientFd = accept(_listenFd, reinterpret_cast<struct sockaddr *>(&clientAddress), &addressLength);
	if (clientFd < 0)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
			std::cerr << "accept() failed" << std::endl;
		return;
	}

	int flags = fcntl(clientFd, F_GETFL, 0);
	if (flags < 0 || fcntl(clientFd, F_SETFL, flags | O_NONBLOCK) < 0)
	{
		close(clientFd);
		return;
	}

	struct pollfd pfd;
	pfd.fd = clientFd;
	pfd.events = POLLIN;
	pfd.revents = 0;
	_pollFds.push_back(pfd);

	_clients.insert(std::make_pair(clientFd, new Client(clientFd)));

	std::cout << "Client connected: " << clientFd << std::endl;
}

/*
Receives data from a client, extracts messages, parses them and sends them to the command handler

Receives up to 4096 bytes from a client
	if bytes are zero, it means client disconnected
	if bytes are less than zero, it means operation failed or couldn't complete
Retrieves corresponding client object
Stores received data into the input buffer
	done in case messages are received sepparately
Retrieves complete IRC messages from the input buffer
Parses the message into command, parameters and trailing parameter (if any)
Executes the command
Checks if the client requested disconnection, for commands like QUIT
	if it does, removes client after processing the command
*/
bool Server::handleClientData(int fd)
{
	char buffer[4096];
	ssize_t bytes = recv(fd, buffer, sizeof(buffer), 0);

	if (bytes == 0)
		return true;

	if (bytes < 0)
		return false;

	std::map<int, Client *>::iterator it = _clients.find(fd);
	if (it == _clients.end())
		return false;

	Client *client = it->second;

	std::cout << "Received from client " << fd << ": " << std::string(buffer, static_cast<std::size_t>(bytes)) << std::endl;

	client->appendInput(std::string(buffer, static_cast<std::size_t>(bytes)));

	std::string line;
	while (client->getNextLine(line))
	{
		Message message;
		if (!message.parse(line))
		{
			std::cout << "Failed to parse: " << line << std::endl;
			continue;
		}

		std::cout << "Command: " << message.getCommand() << std::endl;
		_commandHandler.handle(*client, message);

		if (client->shouldDisconnect())
		{
			removeClient(fd);
			return true;
		}
	}

	return false;
}

/*
Completely removes a client from client management

Finds and removes Client object
Removes file descriptor from _pollFds
Closes socket
*/
void Server::removeClient(int fd)
{
	std::map<int, Client *>::iterator clientIt = _clients.find(fd);
	if (clientIt != _clients.end())
	{
		delete clientIt->second;
		_clients.erase(clientIt);
	}

	for (std::vector<struct pollfd>::iterator it = _pollFds.begin(); it != _pollFds.end(); ++it)
	{
		if (it->fd == fd)
		{
			_pollFds.erase(it);
			break;
		}
	}

	close(fd);
	std::cout << "Client disconnected: " << fd << std::endl;
}

/*Retrieves a client's pointer by its socket description or NULL if not found*/
Client *Server::getClient(int fd)
{
	std::map<int, Client *>::iterator it = _clients.find(fd);
	if (it == _clients.end())
		return NULL;

	return it->second;
}

/*Returns password*/
const std::string &Server::getPassword() const {return _password;}

/*Returns a pointer to the channel or NULL if not found*/
Channel *Server::getChannel(const std::string &name)
{
	std::map<std::string, Channel *>::iterator it = _channels.find(name);
	if (it == _channels.end())
		return NULL;

	return it->second;
}

/*Checks if a channel exists, returns it if it does, creates it if not*/
Channel *Server::getOrCreateChannel(const std::string &name)
{
	Channel *channel = getChannel(name);
	if (channel != NULL)
		return channel;

	channel = new Channel(name);
	_channels[name] = channel;
	return channel;
}

/*Returns a constant referebce to the client map*/
const std::map<int, Client *> &Server::getClients() const {return _clients;}

/*Searches clients to find one with requested nickname and returns it, or NULL if not found*/
Client *Server::findClientByNickname(const std::string &nickname)
{
	for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		if (it->second->getNickname() == nickname)
			return it->second;

	return NULL;
}

/*
Queues outgoing message for a client
	because sockets are non-blocking, unsent data is preserved until socket is ready to receive
*/
void Server::sendToClient(Client &client, const std::string &message)
{
	client.queueOutput(message);
	updatePollEvents(client.getFd());
}

/*
Configures which event should poll() monitor for a particular client
	POLLIN: Socket is ready to receive data
	POLLOUT: Socket is ready to send data
POLLIN is enabled by default
POLLOUT is enabled only when there is outgoing data waiting to be sent
*/
void Server::updatePollEvents(int fd)
{
	Client *client = getClient(fd);
	if (client == NULL)
		return;

	for (std::vector<struct pollfd>::iterator it = _pollFds.begin(); it != _pollFds.end(); ++it)
	{
		if (it->fd == fd)
		{
			it->events = POLLIN;
			if (!client->getOutputBuffer().empty())
				it->events |= POLLOUT;
			return;
		}
	}
}

/*Deletes a channel if there are no members left*/
void Server::removeChannelIfEmpty(const std::string &name)
{
	Channel *channel = getChannel(name);
	if (channel == NULL || !channel->isEmpty())
		return;

	delete channel;
	_channels.erase(name);
}

/*
Removes a disconnected client from every channel it was connected to

Retrieves client information
Constructs a quit message
Notifies other members
Removes client from the channels
Deletes channels that get empty
*/
void Server::removeClientFromChannels(int fd, const std::string &reason)
{
	std::map<std::string, Channel *>::iterator it = _channels.begin();

	while (it != _channels.end())
	{
		Channel *channel = it->second;
		if (channel->hasMember(fd))
		{
			Client *client = getClient(fd);
			if (client != NULL)
			{
				std::string message = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost QUIT :" + reason + "\r\n";
				const std::map<int, Client *> &clients = getClients();

				for (std::map<int, Client *>::const_iterator c = clients.begin(); c != clients.end(); ++c)
				{
					if (c->first != fd && channel->hasMember(c->first))
						sendToClient(*c->second, message);
				}
			}

			channel->removeMember(fd);
		}

		if (channel->isEmpty())
		{
			delete channel;
			_channels.erase(it++);
		}
		else
			++it;
	}
}

/*Completely disconnects client, removes it from channels and deletes it*/
void Server::disconnectClient(int fd, const std::string &reason)
{
	if (_clients.find(fd) == _clients.end())
		return;

	removeClientFromChannels(fd, reason);
	removeClient(fd);
}

/*
Main function
Monitors all sockets and reacts when something happens

poll() monitors every descriptor stored in _pollFds.
	waits indefinitely until until an event occurs or a signal interrupts it
If poll() is interrupted by SIGINT, exits loop
	unexpected errors cause an exception
Iterates over ready descriptors
If a descriptor has no events, moves to the next one
If a ready descriptor is the listening socket, accepts the connection and registers the new client
If a client socket reports an error or invalid descriptor, the server disconnects it
Checks if a client still exists to prevent accessing to nonexistent objects
If a connection has been terminated, server removes the client
If the socket is ready to receive data, retrieves output buffer, trnasmits data, and updates polling events
*/
void Server::run()
{
	while (true)
	{
		int ready = poll(&_pollFds[0], static_cast<nfds_t>(_pollFds.size()), -1);
		if (ready < 0)
		{
			if (errno == EINTR)
				break;
			throw std::runtime_error("poll() failed");
		}

		for (std::size_t i = 0; i < _pollFds.size() && ready > 0;)
		{
			if (_pollFds[i].revents == 0)
			{
				++i;
				continue;
			}

			--ready;

			int fd = _pollFds[i].fd;
			short events = _pollFds[i].revents;

			if (fd == _listenFd)
			{
				if (events & POLLIN)
					acceptClient();

				if (events & (POLLERR | POLLHUP | POLLNVAL))
					throw std::runtime_error("Listening socket failed");

				++i;
				continue;
			}

			if (events & (POLLERR | POLLNVAL))
			{
				disconnectClient(fd, "Connection error");
				continue;
			}

			if (events & POLLIN)
			{
				if (handleClientData(fd))
				{
					disconnectClient(fd, "Connection closed");
					continue;
				}
			}

			if (getClient(fd) == NULL)
				continue;

			if (events & POLLHUP)
			{
				disconnectClient(fd, "Connection closed");
				continue;
			}

			Client *client = getClient(fd);
			if (client != NULL && (events & POLLOUT))
			{
				const std::string &output = client->getOutputBuffer();
				if (!output.empty())
				{
					ssize_t sent = send(fd, output.c_str(), output.size(), 0);
					if (sent == 0)
					{
						disconnectClient(fd, "Connection closed");
						continue;
					}
					if (sent > 0)
						client->consumeOutput(static_cast<std::size_t>(sent));
				}

				updatePollEvents(fd);
			}

			++i;
		}
	}
}