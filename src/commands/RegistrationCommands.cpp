/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RegistrationCommands.cpp                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: manualva <manualva@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/29 17:47:54 by manualva          #+#    #+#             */
/*   Updated: 2026/10/29 18:06:06 by manualva         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CommandHandler.hpp"
#include "Server.hpp"

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

void CommandHandler::handleNick(Client &client, const Message &message)
{
	const std::vector<std::string> &params = message.getParams();

	if (params.empty())
		return sendNumeric(client, 431, ":No nickname given");

	const std::string &nickname = params[0];

	if (nickname.empty() || (!std::isalpha(static_cast<unsigned char>(nickname[0])) && std::string("[]\\`_^{|}").find(nickname[0]) == std::string::npos))
		return sendNumeric(client, 432, nickname + " :Erroneous nickname");

	for (std::size_t i = 1; i < nickname.size(); ++i)
	{
		unsigned char c = static_cast<unsigned char>(nickname[i]);
		if (!std::isalnum(c) && std::string("-[]\\`_^{|}").find(nickname[i]) == std::string::npos)
			return sendNumeric(client, 432, nickname + " :Erroneous nickname");
	}

	for (std::map<int, Client *>::const_iterator it = _server.getClients().begin(); it != _server.getClients().end(); ++it)
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

	client.setNickname(nickname);
	checkRegistration(client);
}

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