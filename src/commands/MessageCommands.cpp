/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   MessageCommands.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: manualva <manualva@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/29 17:47:51 by manualva          #+#    #+#             */
/*   Updated: 2026/10/29 18:06:05 by manualva         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CommandHandler.hpp"
#include "Server.hpp"

void CommandHandler::handlePrivmsg(Client &client, const Message &message)
{
	const std::vector<std::string> &params = message.getParams();

	if (!client.isRegistered())
		return sendNumeric(client, 451, ":You have not registered");
	if (params.empty() || params[0].empty())
		return sendNumeric(client, 411, ":No recipient given (PRIVMSG)");
	if (params.size() < 2)
		return sendNumeric(client, 412, ":No text to send");

	std::string recipients = params[0];
	std::string text = params[1];
	std::string::size_type start = 0;

	while (start < recipients.size())
	{
		std::string::size_type end = recipients.find(',', start);
		std::string target = recipients.substr(start,
			end == std::string::npos ? std::string::npos : end - start);

		if (target.empty())
			sendNumeric(client, 411, ":No recipient given (PRIVMSG)");
		else
		{
			std::string prefix = ":" + client.getNickname() + "!" + client.getUsername() + "@localhost PRIVMSG " + target + " :" + text + "\r\n";

			if (target[0] == '#' || target[0] == '&')
			{
				Channel *channel = _server.getChannel(target);

				if (channel == NULL)
					sendNumeric(client, 403, target + " :No such channel");
				else if (!channel->hasMember(client.getFd()))
					sendNumeric(client, 404, target + " :Cannot send to channel");
				else
				{
					const std::map<int, Client *> &clients = _server.getClients();

					for (std::map<int, Client *>::const_iterator it = clients.begin(); it != clients.end(); ++it)
						if (it->first != client.getFd() && channel->hasMember(it->first))
							_server.sendToClient(*it->second, prefix);
				}
			}
			else
			{
				Client *recipient = _server.findClientByNickname(target);

				if (recipient == NULL)
					sendNumeric(client, 401, target + " :No such nick");
				else
					_server.sendToClient(*recipient, prefix);
			}
		}

		if (end == std::string::npos)
			break;
		start = end + 1;
	}
}