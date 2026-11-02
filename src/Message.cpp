/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Message.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: manualva <manualva@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/27 11:20:14 by manualva          #+#    #+#             */
/*   Updated: 2026/10/28 16:03:56 by manualva         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Message.hpp"

#include <cctype>

Message::Message(){}

Message::Message(const std::string &line){parse(line);}

bool Message::parse(const std::string &line)
{
	_prefix.clear();
	_command.clear();
	_params.clear();

	std::size_t pos = 0;

	while (pos < line.size() && line[pos] == ' ')
		++pos;

	if (pos == line.size())
		return false;

	if (line[pos] == ':')
	{
		std::size_t end = line.find(' ', pos);
		if (end == std::string::npos)
			return false;

		_prefix = line.substr(pos + 1, end - pos - 1);
		pos = end;

		while (pos < line.size() && line[pos] == ' ')
			++pos;

		if (pos == line.size())
			return false;
	}

	std::size_t end = line.find(' ', pos);

	if (end == std::string::npos)
		_command = line.substr(pos);
	else
	{
		_command = line.substr(pos, end - pos);
		pos = end;

		while (pos < line.size())
		{
			while (pos < line.size() && line[pos] == ' ')
				++pos;

			if (pos == line.size())
				break;

			if (line[pos] == ':')
			{
				_params.push_back(line.substr(pos + 1));
				break;
			}

			end = line.find(' ', pos);

			if (end == std::string::npos)
			{
				_params.push_back(line.substr(pos));
				break;
			}

			_params.push_back(line.substr(pos, end - pos));
			pos = end;
		}
	}

	for (std::size_t i = 0; i < _command.size(); ++i)
		_command[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(_command[i])));

	return !_command.empty();
}

const std::string &Message::getPrefix() const{return _prefix;}

const std::string &Message::getCommand() const{return _command;}

const std::vector<std::string> &Message::getParams() const{return _params;}