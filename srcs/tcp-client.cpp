/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   tcp-client.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbourand <mbourand@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/02/23 19:38:29 by nforay            #+#    #+#             */
/*   Updated: 2021/03/01 22:39:46 by mbourand         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ClientSocket.hpp"
#include <string>
#include <iostream>

int	mainb(void)
{
	std::cout << "Starting Client..." << std::endl;
	try
	{
		ClientSocket client("localhost", 8080);
		std::cout << "Client is ready, sending data..." << std::endl;
		try
		{
			std::string	data;
			client << "GET / HTTP/1.1\r\nHost: localhost\r\nUser-Agent: TCP-Client/42.0\r\n\r\n";
		}
		catch(ClientSocket::ClientSocketException&)
		{}
		try
		{
			std::string	data;
			client >> data;
			std::cout << data << std::endl;
		}
		catch(ClientSocket::ClientSocketException &e)
		{
			std::cerr << e.what() << '\n';
		}
	}
	catch(ClientSocket::ClientSocketException &e)
	{
		std::cerr << e.what() << '\n';
	}
	return (0);
}
