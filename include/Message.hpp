/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Message.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: manualva <manualva@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/27 11:19:51 by manualva          #+#    #+#             */
/*   Updated: 2026/10/28 14:51:39 by manualva         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>
#include <vector>
#include <cctype>

class Message
{
private:
	std::string              _prefix;
	std::string              _command;
	std::vector<std::string> _params;

public:
	Message();
	explicit Message(const std::string &line);

	bool parse(const std::string &line);

	const std::string &getPrefix() const;
	const std::string &getCommand() const;
	const std::vector<std::string> &getParams() const;
};

#endif