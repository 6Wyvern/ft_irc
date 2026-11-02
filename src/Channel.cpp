/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: manualva <manualva@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/29 16:06:54 by manualva          #+#    #+#             */
/*   Updated: 2026/10/29 16:24:24 by manualva         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Channel.hpp"

Channel::Channel(const std::string &name) : _name(name), _inviteOnly(false), _topicRestricted(false), _hasKey(false), _hasUserLimit(false), _userLimit(0) {}

Channel::~Channel() {}

const std::string &Channel::getName() const{return _name;}
const std::string &Channel::getTopic() const{return _topic;}
std::size_t Channel::getMemberCount() const{return _members.size();}

void Channel::setTopic(const std::string &topic){_topic = topic;}

bool Channel::hasMember(int fd) const{return _members.find(fd) != _members.end();}
bool Channel::isOperator(int fd) const{return _operators.find(fd) != _operators.end();}
bool Channel::isInvited(int fd) const{return _invited.find(fd) != _invited.end();}

void Channel::addMember(int fd){_members.insert(fd);}
void Channel::removeMember(int fd)
{
	_members.erase(fd);
	_operators.erase(fd);
	_invited.erase(fd);
}

void Channel::addOperator(int fd){_operators.insert(fd);}
void Channel::removeOperator(int fd){_operators.erase(fd);}

void Channel::invite(int fd){_invited.insert(fd);}
void Channel::removeInvitation(int fd){_invited.erase(fd);}

bool Channel::isInviteOnly() const{return _inviteOnly;}
bool Channel::isTopicRestricted() const{return _topicRestricted;}
bool Channel::hasKey() const{return _hasKey;}
bool Channel::hasUserLimit() const{return _hasUserLimit;}
bool Channel::isEmpty() const{return _members.empty();}
std::size_t Channel::getUserLimit() const{return _userLimit;}
const std::string &Channel::getKey() const{return _key;}

void Channel::setInviteOnly(bool value){_inviteOnly = value;}
void Channel::setTopicRestricted(bool value){_topicRestricted = value;}

void Channel::setKey(const std::string &key)
{
	_key = key;
	_hasKey = true;
}
void Channel::removeKey()
{
	_key.clear();
	_hasKey = false;
}
void Channel::setUserLimit(std::size_t limit)
{
	_userLimit = limit;
	_hasUserLimit = true;
}
void Channel::removeUserLimit()
{
	_userLimit = 0;
	_hasUserLimit = false;
}