/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CommandHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: manualva <manualva@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/28 15:03:21 by manualva          #+#    #+#             */
/*   Updated: 2026/10/29 17:52:00 by manualva         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/CommandHandler.hpp"
#include "../include/Server.hpp"

#include <sstream>
#include <iomanip>
#include <cctype>
#include <vector>

CommandHandler::CommandHandler(Server &server): _server(server){}

void CommandHandler::sendNumeric(Client &client, int numeric, const std::string &text)
{
	std::ostringstream number;
	
	number << std::setw(3) << std::setfill('0') << numeric;
	std::string nickname = client.getNickname().empty() ? "*" : client.getNickname();
	_server.sendToClient(client, ":localhost " + number.str() + " " + nickname + " " + text + "\r\n");
}

void CommandHandler::checkRegistration(Client &client)
{
	if (!client.isRegistered() && client.hasPassed() && !client.getNickname().empty() && client.hasUserCommand())
	{
		client.setRegistered(true);
		sendNumeric(client, 1, ":Welcome to the IRC Network " + client.getNickname());
	}
}

void CommandHandler::handle(Client &client, const Message &message)
{
	const std::string &command = message.getCommand();

	if (command == "PASS")
		handlePass(client, message);
	else if (command == "NICK")
		handleNick(client, message);
	else if (command == "USER")
		handleUser(client, message);
	else if (command == "JOIN")
		handleJoin(client, message);
	else if (command == "PRIVMSG")
		handlePrivmsg(client, message);
	else if (command == "PART")
		handlePart(client, message);
	else if (command == "QUIT")
		handleQuit(client, message);
	else if (command == "KICK")
		handleKick(client, message);
	else if (command == "INVITE")
		handleInvite(client, message);
	else if (command == "TOPIC")
		handleTopic(client, message);
	else if (command == "MODE")
		handleMode(client, message);
	else
		sendNumeric(client, 421, command + " :Unknown command");
}