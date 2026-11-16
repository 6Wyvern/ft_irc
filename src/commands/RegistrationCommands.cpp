/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RegistrationCommands.cpp                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: manualva <manualva@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/29 17:47:54 by manualva          #+#    #+#             */
/*   Updated: 2026/11/16 18:37:24 by manualva         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CommandHandler.hpp"
#include "Server.hpp"

/*
PASS command
Inputs password

If client is already registered, sends error message 462
If no password is provided, sends error message 461
If password is incorrect, sends error message 464
*/
void CommandHandler::handlePass(Client &client, const Message &message)
{
	const std::vector<std::string> &params = message.getParams();

	if (client.isRegistered())
		return sendNumeric(client, 462, ":You may not reregister");
	if (params.empty())
		return sendNumeric(client, 461, "PASS :Not enough parameters");
	if (params[0] != _server.getPassword())
		return sendNumeric(client, 464, ":Password incorrect");

	client.setHasPassed(true);
	checkRegistration(client);
}

/*
NICK command
Handles nickname

If no nickname is provided, sends error message 431
If nickname has more than 9 characters or non-alphanumeric characters are used, sends error message 432
If nickname is already in use, error sends message 433
*/
void CommandHandler::handleNick(Client &client, const Message &message)
{
	const std::vector<std::string> &params = message.getParams();

	if (params.empty())
		return sendNumeric(client, 431, ":No nickname given");

	const std::string &nickname = params[0];

	if (nickname.empty() || nickname.size() > 9 || (!std::isalpha(static_cast<unsigned char>(nickname[0])) && std::string("[]\\`_^{|}").find(nickname[0]) == std::string::npos))
		return sendNumeric(client, 432, nickname + " :Erroneous nickname");

	for (std::size_t i = 1; i < nickname.size(); ++i)
	{
		unsigned char c = static_cast<unsigned char>(nickname[i]);
		if (!std::isalnum(c) && std::string("-[]\\`_^{|}").find(nickname[i]) == std::string::npos)
			return sendNumeric(client, 432, nickname + " :Erroneous nickname");
	}

	for (std::map<int, Client *>::const_iterator it = _server.getClients().begin();
		it != _server.getClients().end(); ++it)
	{
		if (it->second == &client)
			continue;

		std::string existing = it->second->getNickname();
		std::string requested = nickname;

		for (std::size_t i = 0; i < existing.size(); ++i)
			existing[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(existing[i])));
		for (std::size_t i = 0; i < requested.size(); ++i)
			requested[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(requested[i])));

		if (!existing.empty() && existing == requested)
			return sendNumeric(client, 433, nickname + " :Nickname is already in use");
	}

	if (client.isRegistered())
	{
		std::string reply = ":" + client.getNickname() + "!" + client.getUsername() + "@localhost NICK :" + nickname + "\r\n";

		const std::map<int, Client *> &clients = _server.getClients();
		for (std::map<int, Client *>::const_iterator it = clients.begin(); it != clients.end(); ++it)
		{
			if (it->second == &client)
			{
				_server.sendToClient(client, reply);
				continue;
			}
		}
	}

	client.setNickname(nickname);
	checkRegistration(client);
}

/*
USER command
Registers user

If already registered, sends error message 462
If message is empty or doesn't have enough parameters, sends error message 461
*/
void CommandHandler::handleUser(Client &client, const Message &message)
{
	const std::vector<std::string> &params = message.getParams();

	if (client.isRegistered())
		return sendNumeric(client, 462, ":You may not reregister");
	if (params.size() < 4)
		return sendNumeric(client, 461, "USER :Not enough parameters");
	if (params[0].empty())
		return sendNumeric(client, 461, "USER :Not enough parameters");

	client.setUsername(params[0]);
	client.setRealname(params[3]);
	client.setHasUserCommand(true);
	checkRegistration(client);
}