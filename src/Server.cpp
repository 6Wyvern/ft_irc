/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: manualva <manualva@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/26 15:57:49 by manualva          #+#    #+#             */
/*   Updated: 2026/11/03 15:51:47 by manualva         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"
#include <signal.h>

Server::Server(int port, const std::string &password): _listenFd(-1), _port(port), _password(password), _commandHandler(*this){setupSocket();}

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

void Server::setupSocket()
{
	signal(SIGPIPE, SIG_IGN);

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
	}

	return false;
}

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

Client *Server::getClient(int fd)
{
	std::map<int, Client *>::iterator it = _clients.find(fd);
	if (it == _clients.end())
		return NULL;

	return it->second;
}
const std::string &Server::getPassword() const {return _password;}
Channel *Server::getChannel(const std::string &name)
{
	std::map<std::string, Channel *>::iterator it = _channels.find(name);
	if (it == _channels.end())
		return NULL;

	return it->second;
}
Channel *Server::getOrCreateChannel(const std::string &name)
{
	Channel *channel = getChannel(name);
	if (channel != NULL)
		return channel;

	channel = new Channel(name);
	_channels[name] = channel;
	return channel;
}
const std::map<int, Client *> &Server::getClients() const {return _clients;}

Client *Server::findClientByNickname(const std::string &nickname)
{
	for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		if (it->second->getNickname() == nickname)
			return it->second;

	return NULL;
}

void Server::sendToClient(Client &client, const std::string &message)
{
	client.queueOutput(message);
	updatePollEvents(client.getFd());
}

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

void Server::removeChannelIfEmpty(const std::string &name)
{
	Channel *channel = getChannel(name);
	if (channel == NULL || !channel->isEmpty())
		return;

	delete channel;
	_channels.erase(name);
}

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

void Server::disconnectClient(int fd, const std::string &reason)
{
	if (_clients.find(fd) == _clients.end())
		return;

	removeClientFromChannels(fd, reason);
	removeClient(fd);
}

void Server::run()
{
	while (true)
	{
		int ready = poll(&_pollFds[0], static_cast<nfds_t>(_pollFds.size()), -1);
		if (ready < 0)
		{
			if (errno == EINTR)
				continue;
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