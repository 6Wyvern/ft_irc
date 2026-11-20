/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   MemberCommands.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: manualva <manualva@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/29 17:47:46 by manualva          #+#    #+#             */
/*   Updated: 2026/11/18 16:42:27 by manualva         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CommandHandler.hpp"
#include "Server.hpp"

/*
JOIN command
Joins a channel and prints a list of members after joining

If user isn't registered, sends error message 451
If no channel is specified, sends error message 461
If channel name has less than 2 characters or it doesn't start with # or &, sends error message 476
If a channel is invite-only (+i) and you aren't invited, sends error message 473
If a channel has a paswword key (+k) and password is incorrect, sends error message 475
If a channel has a user limit (+l) and joinig would exceed the limit, sends error message 471
*/
void CommandHandler::handleJoin(Client &client, const Message &message)
{
	const std::vector<std::string> &params = message.getParams();

	if (!client.isRegistered())
		return sendNumeric(client, 451, ":You have not registered");
	if (params.empty())
		return sendNumeric(client, 461, "JOIN :Not enough parameters");

	std::string names = params[0];
	std::string::size_type start = 0;

	while (start < names.size())
	{
		std::string::size_type end = names.find(',', start);
		std::string name = names.substr(start, end == std::string::npos ? std::string::npos : end - start);

		if (name.empty() || name.size() < 2 || (name[0] != '#' && name[0] != '&'))
			sendNumeric(client, 476, name + " :Bad Channel Mask");
		else
		{
			Channel *channel = _server.getChannel(name);

			if (channel != NULL && channel->hasMember(client.getFd())){}
			else if (channel != NULL && channel->isInviteOnly() && !channel->isInvited(client.getFd()))
				sendNumeric(client, 473, name + " :Cannot join channel (+i)");
			else if (channel != NULL && channel->hasKey() && (params.size() < 2 || params[1] != channel->getKey()))
				sendNumeric(client, 475, name + " :Cannot join channel (+k)");
			else if (channel != NULL && channel->hasUserLimit() && channel->getMemberCount() >= channel->getUserLimit())
				sendNumeric(client, 471, name + " :Cannot join channel (+l)");
			else
			{
				bool created = (channel == NULL);
				channel = _server.getOrCreateChannel(name);
				channel->addMember(client.getFd());

				if (created)
					channel->addOperator(client.getFd());

				channel->removeInvitation(client.getFd());

				std::string prefix = ":" + client.getNickname() + "!" + client.getUsername() + "@localhost JOIN " + name + "\r\n";

				const std::map<int, Client *> &clients = _server.getClients();

				for (std::map<int, Client *>::const_iterator it = clients.begin(); it != clients.end(); ++it)
					if (channel->hasMember(it->first))
						_server.sendToClient(*it->second, prefix);

				if (!channel->getTopic().empty())
					sendNumeric(client, 332, name + " :" + channel->getTopic());

				std::string members;

				for (std::map<int, Client *>::const_iterator it = clients.begin(); it != clients.end(); ++it)
				{
					if (channel->hasMember(it->first))
					{
						if (!members.empty())
							members += " ";
						if (channel->isOperator(it->first))
							members += "@";
						members += it->second->getNickname();
					}
				}

				sendNumeric(client, 353, "= " + name + " :" + members);
				sendNumeric(client, 366, name + " :End of /NAMES list");
			}
		}

		if (end == std::string::npos)
			break;
		start = end + 1;
	}
}

/*
PART command
Leaves a channel and leaves a message

If user isn't registered, sends error message 451
If no parameters are specified, sends error message 461
If channel doesn't exist, sends error message 403
If user isn't in the channel, sends error message 442
*/
void CommandHandler::handlePart(Client &client, const Message &message)
{
	const std::vector<std::string> &params = message.getParams();

	if (!client.isRegistered())
		return sendNumeric(client, 451, ":You have not registered");
	if (params.empty())
		return sendNumeric(client, 461, "PART :Not enough parameters");

	std::string names = params[0];
	std::string reason = params.size() > 1 ? params[1] : client.getNickname();
	std::string::size_type start = 0;

	while (start < names.size())
	{
		std::string::size_type end = names.find(',', start);
		std::string name = names.substr(start, end == std::string::npos
			? std::string::npos : end - start);
		Channel *channel = _server.getChannel(name);

		if (channel == NULL)
			sendNumeric(client, 403, name + " :No such channel");
		else if (!channel->hasMember(client.getFd()))
			sendNumeric(client, 442, name + " :You're not on that channel");
		else
		{
			std::string reply = ":" + client.getNickname() + "!" + client.getUsername() + "@localhost PART " + name + " :" + reason + "\r\n";
			const std::map<int, Client *> &clients = _server.getClients();

			for (std::map<int, Client *>::const_iterator it = clients.begin(); it != clients.end(); ++it)
				if (channel->hasMember(it->first))
					_server.sendToClient(*it->second, reply);

			channel->removeMember(client.getFd());
			_server.removeChannelIfEmpty(name);
		}

		if (end == std::string::npos)
			break;
		start = end + 1;
	}
}

/*
QUIT command
Disconnects from the server
*/
void CommandHandler::handleQuit(Client &client, const Message &message)
{
	const std::vector<std::string> &params = message.getParams();
	std::string reason = params.empty() ? client.getNickname() : params[0];

	_server.removeClientFromChannels(client.getFd(), reason);
	client.requestDisconnect();
}

/*
INVITE command
Invites a user to the channel and allows joining if invite-only, notifies target (+i)

If user isn't registered, sends error message 451
If no parameters are specified, sends error message 461
If target's nick doesn't exist, sends error message 401
If channel doesn't exist, sends error message 403
If user isn't in the channel, sends error message 442
If target is already on the channel, sends error message 443
If channel is invite-only (+i) and user isn't an operator (+o), sends error message 482
*/
void CommandHandler::handleInvite(Client &client, const Message &message)
{
	const std::vector<std::string> &params = message.getParams();

	if (!client.isRegistered())
		return sendNumeric(client, 451, ":You have not registered");
	if (params.size() < 2)
		return sendNumeric(client, 461, "INVITE :Not enough parameters");

	Client *target = _server.findClientByNickname(params[0]);
	if (target == NULL)
		return sendNumeric(client, 401, params[0] + " :No such nick/channel");

	Channel *channel = _server.getChannel(params[1]);
	if (channel == NULL)
		return sendNumeric(client, 403, params[1] + " :No such channel");
	if (!channel->hasMember(client.getFd()))
		return sendNumeric(client, 442, params[1] + " :You're not on that channel");
	if (channel->hasMember(target->getFd()))
		return sendNumeric(client, 443, params[0] + " " + params[1] + " :is already on channel");
	if (channel->isInviteOnly() && !channel->isOperator(client.getFd()))
		return sendNumeric(client, 482, params[1] + " :You're not channel operator");

	channel->invite(target->getFd());

	sendNumeric(client, 341, params[0] + " " + params[1]);
	_server.sendToClient(*target, ":" + client.getNickname() + "!" + client.getUsername() + "@localhost INVITE " + target->getNickname() + " " + params[1] + "\r\n");
}

/*
KICK command
Removes a user from the channel with a message

If user isn't registered, sends error message 451
If no parameters are specified, sends errormessage 461
If channel doesn't exist, sends error message 403
If user isn't in the channel, sends error message 442
If user isn't an operator (+o), sends error message 482
If target's nick doesn't exist, sends error message 401
If target isn't on the channel, sends error message 441
*/
void CommandHandler::handleKick(Client &client, const Message &message)
{
	const std::vector<std::string> &params = message.getParams();

	if (!client.isRegistered())
		return sendNumeric(client, 451, ":You have not registered");
	if (params.size() < 2)
		return sendNumeric(client, 461, "KICK :Not enough parameters");

	Channel *channel = _server.getChannel(params[0]);
	if (channel == NULL)
		return sendNumeric(client, 403, params[0] + " :No such channel");
	if (!channel->hasMember(client.getFd()))
		return sendNumeric(client, 442, params[0] + " :You're not on that channel");
	if (!channel->isOperator(client.getFd()))
		return sendNumeric(client, 482, params[0] + " :You're not channel operator");

	Client *target = _server.findClientByNickname(params[1]);
	if (target == NULL)
		return sendNumeric(client, 401, params[1] + " :No such nick/channel");
	if (!channel->hasMember(target->getFd()))
		return sendNumeric(client, 441, params[1] + " " + params[0] + " :They aren't on that channel");

	std::string reason = params.size() > 2 ? params[2] : client.getNickname();
	std::string reply = ":" + client.getNickname() + "!" + client.getUsername() + "@localhost KICK " + params[0] + " " + target->getNickname() + " :" + reason + "\r\n";

	const std::map<int, Client *> &clients = _server.getClients();
	for (std::map<int, Client *>::const_iterator it = clients.begin(); it != clients.end(); ++it)
		if (channel->hasMember(it->first))
			_server.sendToClient(*it->second, reply);

	channel->removeMember(target->getFd());
	_server.removeChannelIfEmpty(params[0]);
}
