/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: manualva <manualva@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/26 16:34:15 by manualva          #+#    #+#             */
/*   Updated: 2026/10/28 16:04:19 by manualva         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Client.hpp"

Client::Client(int fd) : _fd(fd), _hasPassed(false), _registered(false), _hasUserCommand(false){}

Client::~Client(){}

int Client::getFd() const{return _fd;}
const std::string &Client::getNickname() const{return _nickname;}
const std::string &Client::getUsername() const{return _username;}
const std::string &Client::getRealname() const{return _realname;}
const std::string &Client::getInputBuffer() const{return _inputBuffer;}
const std::string &Client::getOutputBuffer() const{return _outputBuffer;}

bool Client::hasPassed() const{return _hasPassed;}
bool Client::isRegistered() const{return _registered;}
bool Client::hasUserCommand() const{return _hasUserCommand;}

void Client::setNickname(const std::string &nickname){_nickname = nickname;}
void Client::setUsername(const std::string &username){_username = username;}
void Client::setRealname(const std::string &realname){_realname = realname;}
void Client::setHasPassed(bool value){_hasPassed = value;}
void Client::setRegistered(bool value){_registered = value;}
void Client::setHasUserCommand(bool value){_hasUserCommand = value;}

void Client::appendInput(const std::string &data){_inputBuffer += data;}

bool Client::getNextLine(std::string &line)
{
	std::size_t position = _inputBuffer.find('\n');

	if (position == std::string::npos)
		return false;

	line = _inputBuffer.substr(0, position);

	if (!line.empty() && line[line.size() - 1] == '\r')
		line.erase(line.size() - 1);

	_inputBuffer.erase(0, position + 1);

	return true;
}

void Client::queueOutput(const std::string &data){_outputBuffer += data;}

void Client::consumeOutput(std::size_t length)
{
    if (length >= _outputBuffer.size())
        _outputBuffer.clear();
    else
        _outputBuffer.erase(0, length);
}