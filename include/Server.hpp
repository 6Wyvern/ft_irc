/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: manualva <manualva@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/26 15:57:47 by manualva          #+#    #+#             */
/*   Updated: 2026/11/03 15:47:14 by manualva         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include "Client.hpp"
#include "Message.hpp"
#include "Channel.hpp"
#include "CommandHandler.hpp"

#include <map>
#include <poll.h>

#include <iostream>
#include <stdexcept>
#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class Server
{
private:
	int _listenFd;
	int _port;
	std::string _password;
	std::vector<struct pollfd> _pollFds;
	std::map<int, Client *> _clients;
	std::map<std::string, Channel *> _channels;
	CommandHandler _commandHandler;

	Server(const Server &other);
	Server &operator=(const Server &other);

	void setupSocket();
	void acceptClient();
	bool handleClientData(int fd);
	void removeClient(int fd);

public:
	Server(int port, const std::string &password);
	~Server();

	void run();

	Client *getClient(int fd);
	const std::string &getPassword() const;
	Channel *getChannel(const std::string &name);
	Channel *getOrCreateChannel(const std::string &name);
	const std::map<int, Client *> &getClients() const;

	Client *findClientByNickname(const std::string &nickname);
	void sendToClient(Client &client, const std::string &message);
	void updatePollEvents(int fd);
	void removeChannelIfEmpty(const std::string &name);
	void removeClientFromChannels(int fd, const std::string &reason);
	void disconnectClient(int fd, const std::string &reason);
};

#endif