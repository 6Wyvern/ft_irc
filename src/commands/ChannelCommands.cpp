/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ChannelCommands.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: manualva <manualva@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/29 17:47:49 by manualva          #+#    #+#             */
/*   Updated: 2026/10/29 18:05:59 by manualva         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CommandHandler.hpp"
#include "Server.hpp"

void CommandHandler::handleTopic(Client &client, const Message &message)
{
	const std::vector<std::string> &params = message.getParams();

	if (!client.isRegistered())
		return sendNumeric(client, 451, ":You have not registered");
	if (params.empty())
		return sendNumeric(client, 461, "TOPIC :Not enough parameters");

	Channel *channel = _server.getChannel(params[0]);

	if (channel == NULL)
		return sendNumeric(client, 403, params[0] + " :No such channel");
	if (!channel->hasMember(client.getFd()))
		return sendNumeric(client, 442, params[0] + " :You're not on that channel");

	if (params.size() == 1)
	{
		if (channel->getTopic().empty())
			sendNumeric(client, 331, params[0] + " :No topic is set");
		else
			sendNumeric(client, 332, params[0] + " :" + channel->getTopic());
		return;
	}

	if (channel->isTopicRestricted() && !channel->isOperator(client.getFd()))
		return sendNumeric(client, 482, params[0] + " :You're not channel operator");

	channel->setTopic(params[1]);

	std::string reply = ":" + client.getNickname() + "!" + client.getUsername() + "@localhost TOPIC " + params[0] + " :" + params[1] + "\r\n";

	const std::map<int, Client *> &clients = _server.getClients();

	for (std::map<int, Client *>::const_iterator it = clients.begin(); it != clients.end(); ++it)
		if (channel->hasMember(it->first))
			_server.sendToClient(*it->second, reply);
}

void CommandHandler::handleMode(Client &client, const Message &message)
{
	const std::vector<std::string> &params = message.getParams();

	if (!client.isRegistered())
		return sendNumeric(client, 451, ":You have not registered");
	if (params.empty())
		return sendNumeric(client, 461, "MODE :Not enough parameters");

	Channel *channel = _server.getChannel(params[0]);
	if (channel == NULL)
		return sendNumeric(client, 403, params[0] + " :No such channel");
	if (!channel->hasMember(client.getFd()))
		return sendNumeric(client, 442, params[0] + " :You're not on that channel");

	if (params.size() == 1)
	{
		std::string modes = "+";
		std::string arguments;

		if (channel->isInviteOnly())
			modes += "i";
		if (channel->isTopicRestricted())
			modes += "t";
		if (channel->hasKey())
		{
			modes += "k";
			arguments += " " + channel->getKey();
		}
		if (channel->hasUserLimit())
		{
			modes += "l";
			std::ostringstream limit;
			limit << channel->getUserLimit();
			arguments += " " + limit.str();
		}

		sendNumeric(client, 324, params[0] + " " + modes + arguments);
		return;
	}

	if (!channel->isOperator(client.getFd()))
		return sendNumeric(client, 482, params[0] + " :You're not channel operator");

	std::string modeString = params[1];
	bool adding = true;
	std::size_t argIndex = 2;

	for (std::size_t i = 0; i < modeString.size(); ++i)
	{
		char mode = modeString[i];

		if (mode == '+')
		{
			adding = true;
			continue;
		}
		if (mode == '-')
		{
			adding = false;
			continue;
		}

		bool changed = false;
		std::string modeArg;

		if (mode == 'i')
		{
			if (channel->isInviteOnly() != adding)
			{
				channel->setInviteOnly(adding);
				changed = true;
			}
		}
		else if (mode == 't')
		{
			if (channel->isTopicRestricted() != adding)
			{
				channel->setTopicRestricted(adding);
				changed = true;
			}
		}
		else if (mode == 'k')
		{
			if (adding)
			{
				if (argIndex >= params.size())
				{
					sendNumeric(client, 461, "MODE :Not enough parameters");
					break;
				}
				if (channel->hasKey())
				{
					sendNumeric(client, 467, params[0] + " :Channel key already set");
					++argIndex;
					continue;
				}
				modeArg = params[argIndex++];
				channel->setKey(modeArg);
				changed = true;
			}
			else if (channel->hasKey())
			{
				channel->removeKey();
				changed = true;
			}
		}
		else if (mode == 'o')
		{
			if (argIndex >= params.size())
			{
				sendNumeric(client, 461, "MODE :Not enough parameters");
				break;
			}

			modeArg = params[argIndex++];
			Client *target = _server.findClientByNickname(modeArg);

			if (target == NULL)
			{
				sendNumeric(client, 401, modeArg + " :No such nick");
				continue;
			}
			if (!channel->hasMember(target->getFd()))
			{
				sendNumeric(client, 441, modeArg + " " + params[0] + " :They aren't on that channel");
				continue;
			}

			if (adding && !channel->isOperator(target->getFd()))
			{
				channel->addOperator(target->getFd());
				changed = true;
			}
			else if (!adding && channel->isOperator(target->getFd()))
			{
				channel->removeOperator(target->getFd());
				changed = true;
			}
		}
		else if (mode == 'l')
		{
			if (adding)
			{
				if (argIndex >= params.size())
				{
					sendNumeric(client, 461, "MODE :Not enough parameters");
					break;
				}

				modeArg = params[argIndex++];
				bool valid = !modeArg.empty();

				for (std::size_t j = 0; j < modeArg.size(); ++j)
				{
					if (!std::isdigit(static_cast<unsigned char>(modeArg[j])))
						valid = false;
				}

				char *end = NULL;
				unsigned long limit = std::strtoul(modeArg.c_str(), &end, 10);

				if (!valid || end == modeArg.c_str() || *end != '\0' || limit == 0 || static_cast<unsigned long>(static_cast<std::size_t>(limit)) != limit)
				{
					sendNumeric(client, 696, params[0] + " l " + modeArg + " :Invalid mode parameter");
					continue;
				}

				if (!channel->hasUserLimit() || channel->getUserLimit() != limit)
				{
					channel->setUserLimit(static_cast<std::size_t>(limit));
					changed = true;
				}
			}
			else if (channel->hasUserLimit())
			{
				channel->removeUserLimit();
				changed = true;
			}
		}
		else
		{
			sendNumeric(client, 472, std::string(1, mode) + " :is unknown mode char to me");
			continue;
		}

		if (changed)
		{
			std::string reply = ":" + client.getNickname() + "!" + client.getUsername() + "@localhost MODE " + params[0] + " " + (adding ? "+" : "-") + mode;

			if (!modeArg.empty())
				reply += " " + modeArg;
			reply += "\r\n";

			const std::map<int, Client *> &clients = _server.getClients();

			for (std::map<int, Client *>::const_iterator it = clients.begin(); it != clients.end(); ++it)
				if (channel->hasMember(it->first))
					_server.sendToClient(*it->second, reply);
		}
	}
}