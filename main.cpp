/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: manualva <manualva@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/26 16:05:52 by manualva          #+#    #+#             */
/*   Updated: 2026/10/26 17:57:20 by manualva         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "include/Server.hpp"

#include <iostream>
#include <string>
#include <cstdlib>
#include <exception>

static bool isValidPort(const char *str)
{
    char *end;
    long value;

    if (str[0] == '\0')
        return false;

    value = std::strtol(str, &end, 10);

    if (*end != '\0')
        return false;

    if (value < 1 || value > 65535)
        return false;

    return true;
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: ./ircserv <port> <password>"
                  << std::endl;
        return 1;
    }

    if (!isValidPort(argv[1]))
    {
        std::cerr << "Error: invalid port" << std::endl;
        return 1;
    }

    if (argv[2][0] == '\0')
    {
        std::cerr << "Error: password cannot be empty"
                  << std::endl;
        return 1;
    }

    try
    {
        int port = static_cast<int>(std::strtol(argv[1], NULL, 10));
        Server server(port, argv[2]);
        server.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}