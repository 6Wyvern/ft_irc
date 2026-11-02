/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CommandHandler.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: manualva <manualva@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/28 15:02:53 by manualva          #+#    #+#             */
/*   Updated: 2026/10/29 18:05:50 by manualva         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef COMMANDHANDLER_HPP
#define COMMANDHANDLER_HPP

#include "Client.hpp"
#include "Message.hpp"

#include <string>
#include <unistd.h>
#include <sstream>
#include <cstdlib>
#include <cctype>

class Server;

class CommandHandler
{
private:
    Server &_server;

public:
	explicit CommandHandler(Server &server);

	void handle(Client &client, const Message &message);

	void handlePass(Client &client, const Message &message);
	void handleNick(Client &client, const Message &message);
	void handleUser(Client &client, const Message &message);

	void handleJoin(Client &client, const Message &message);
	void handlePrivmsg(Client &client, const Message &message);
	void handlePart(Client &client, const Message &message);
	void handleQuit(Client &client, const Message &message);

	void handleKick(Client &client, const Message &message);
	void handleInvite(Client &client, const Message &message);
	void handleTopic(Client &client, const Message &message);
	void handleMode(Client &client, const Message &message);

	void sendNumeric(Client &client, int numeric, const std::string &text);
	void checkRegistration(Client &client);
};

#endif