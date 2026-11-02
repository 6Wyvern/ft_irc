/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: manualva <manualva@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/28 15:33:37 by manualva          #+#    #+#             */
/*   Updated: 2026/10/30 09:26:12 by manualva         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <set>
#include <map>
#include <cstddef>

class Channel
{
private:
    std::string _name;
    std::string _topic;
    std::string _key;

    std::set<int> _members;
    std::set<int> _operators;
    std::set<int> _invited;

    bool _inviteOnly;
    bool _topicRestricted;
    bool _hasKey;
    bool _hasUserLimit;
    std::size_t _userLimit;

public:
    explicit Channel(const std::string &name);
    ~Channel();

    const std::string &getName() const;
    const std::string &getTopic() const;
	std::size_t getMemberCount() const;

    void setTopic(const std::string &topic);

    bool hasMember(int fd) const;
    bool isOperator(int fd) const;
    bool isInvited(int fd) const;

    void addMember(int fd);
    void removeMember(int fd);

    void addOperator(int fd);
    void removeOperator(int fd);

    void invite(int fd);
    void removeInvitation(int fd);

	bool isInviteOnly() const;
	bool isTopicRestricted() const;
	bool hasKey() const;
	bool hasUserLimit() const;
	bool isEmpty() const;
	
	std::size_t getUserLimit() const;
	const std::string &getKey() const;
	void setUserLimit(std::size_t limit);
	void setKey(const std::string &key);
	void removeKey();
	void removeUserLimit();

	void setInviteOnly(bool value);
	void setTopicRestricted(bool value);
};

#endif