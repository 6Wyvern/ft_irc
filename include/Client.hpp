/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: manualva <manualva@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/26 16:34:18 by manualva          #+#    #+#             */
/*   Updated: 2026/11/03 15:53:12 by manualva         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <cstddef>

class Client
{
private:
	int         _fd;
	std::string _nickname;
	std::string _username;
	std::string _inputBuffer;
	std::string _outputBuffer;
	bool        _hasPassed;
	bool        _registered;
	std::string _realname;
	bool        _hasUserCommand;

	Client(const Client &other);
	Client &operator=(const Client &other);

public:
	explicit Client(int fd);
	~Client();

	int getFd() const;

	const std::string &getNickname() const;
	const std::string &getUsername() const;
	const std::string &getRealname() const;
	const std::string &getInputBuffer() const;
	const std::string &getOutputBuffer() const;

	bool hasPassed() const;
	bool isRegistered() const;
	bool hasUserCommand() const;

	void setNickname(const std::string &nickname);
	void setUsername(const std::string &username);
	void setHasPassed(bool value);
	void setRegistered(bool value);
	void setRealname(const std::string &realname);
	void setHasUserCommand(bool value);

	void appendInput(const std::string &data);
	bool getNextLine(std::string &line);

	void queueOutput(const std::string &data);
	void consumeOutput(std::size_t length);
};

#endif