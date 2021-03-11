/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   tcp-server.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nforay <nforay@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/02/16 01:13:41 by nforay            #+#    #+#             */
/*   Updated: 2021/03/11 19:59:54 by nforay           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Webserv.hpp"
#include "Response.hpp"
#include <string>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <list>
#include <string.h>
#include <errno.h>
#include <signal.h>

#ifndef DEBUG
# define DEBUG 0
#endif

bool	g_run = true;

std::string string_to_hex(const std::string& input)
{
    static const char hex_digits[] = "0123456789ABCDEF";

    std::string output;
    output.reserve(input.length() * 2);
	for(std::string::size_type i = 0; i < input.size(); ++i)
    {
        output.push_back(hex_digits[input[i] >> 4]);
        output.push_back(hex_digits[input[i] & 15]);
		output.push_back(' ');
    }
    return output;
}

std::string string_strip_undisplayable(const std::string& input)
{
    std::string output;
    output.reserve(input.length());
	for(std::string::size_type i = 0; i < input.size(); ++i)
    {
        if (std::isprint(input[i]))
			output.push_back(input[i]);
		else
			output.push_back(' ');
    }
    return output;
}

int	main(void)
{
	Logger::setMode(NORMAL);
	Logger::print("Webserv is starting...", NULL, INFO, SILENT);
	sighandler();
	try
	{
		ServerSocket					server(8080);
		fd_set							read_sockets, read_sockets_z, write_sockets, write_sockets_z, error_sockets, error_sockets_z;
		std::list<Client>				clients;
		std::list<Client>::iterator		it;
		std::list<Client>::iterator		end;

		Logger::print("Webserv is ready, waiting for connection...", NULL, SUCCESS, SILENT);
		FD_ZERO(&read_sockets_z);
		FD_ZERO(&write_sockets_z);
		FD_ZERO(&error_sockets_z);
		FD_SET(server.GetSocket(), &read_sockets_z);
		FD_SET(server.GetSocket(), &write_sockets_z);
		FD_SET(server.GetSocket(), &error_sockets_z);
		while (g_run)
		{
			it = clients.begin();
			end = clients.end();
			int highestFd = server.GetSocket();
			for (; it != end; ++it)
			{
				FD_SET((*it).sckt->GetSocket(), &read_sockets_z);
				FD_SET((*it).sckt->GetSocket(), &write_sockets_z);
				FD_SET((*it).sckt->GetSocket(), &error_sockets_z);
				if ((*it).sckt->GetSocket() > highestFd)
					highestFd = (*it).sckt->GetSocket();
			}
			read_sockets = read_sockets_z;
			write_sockets = write_sockets_z;
			error_sockets = error_sockets_z;
			if (select(highestFd + 1, &read_sockets, &write_sockets, &error_sockets, NULL) < 0)
			{
				if (g_run)
					Logger::print("Error select(): "+std::string(strerror(errno)), NULL, ERROR, NORMAL);
				continue;
			}
			else
			{
				if (FD_ISSET(server.GetSocket(), &read_sockets))
				{
					Client	new_client;
					new_client.sckt = new ServerSocket;
					new_client.req = new Request;
					server.Accept(*new_client.sckt);
					clients.push_back(new_client);
					Logger::print("New Client Connected", NULL, SUCCESS, NORMAL);
				}
				else
				{
					it = clients.begin();
					end = clients.end();
					while (!clients.empty() && it != end)
					{
						bool	error = false;
						if (FD_ISSET((*it).sckt->GetSocket(), &error_sockets))
						{
							error = true;
							Logger::print("FD Error flagged by Select", NULL, WARNING, NORMAL);
						}
						else if (!error && FD_ISSET((*it).sckt->GetSocket(), &read_sockets))
						{
							std::string	data;
							try
							{
								*(*it).sckt >> data;
								(*it).req->append(data);
								if (DEBUG)
									std::cout << "\e[35m" << std::setfill(' ') << std::setw(MAX_RECIEVE * 2 + MAX_RECIEVE) << string_to_hex(data) << "\e[33m\t" << string_strip_undisplayable(data) << "\e[39m" << std::endl;
							}
							catch(ServerSocket::ServerSocketException &e)
							{
								Logger::print(e.what(), NULL, ERROR, NORMAL);
								error = true;
							}
							catch(std::invalid_argument &e)
							{
								Response response(400, "Bad Request");
								*(*it).sckt << response.getResponseText();
								error = true;
								Logger::print(e.what(), NULL, ERROR, NORMAL);
							}
							if (!error && (*it).req->isfinished() && FD_ISSET((*it).sckt->GetSocket(), &write_sockets))
							{
								try
								{
									Response response = (*it).req->_method->process(*(*it).req);
									*(*it).sckt << response.getResponseText();
									if (DEBUG)
										std::cout << *(*it).req << std::endl;
									if (response.getCode() != 200)
										error = true;
									Logger::print("Success", NULL, SUCCESS, NORMAL);
								}
								catch(ServerSocket::ServerSocketException &e)
								{
									Logger::print(e.what(), NULL, ERROR, NORMAL);
								}
								delete (*it).req;
								(*it).req = new Request;
							}
						}
						if (error)
						{
							Logger::print("Client Disconnected", NULL, SUCCESS, VERBOSE);
							FD_CLR((*it).sckt->GetSocket(), &read_sockets_z);
							FD_CLR((*it).sckt->GetSocket(), &write_sockets_z);
							FD_CLR((*it).sckt->GetSocket(), &error_sockets_z);
							delete (*it).sckt;
							delete (*it).req;
							it = clients.erase(it);
						}
						else
						{
							++it;
						}
					}
				}
			}
		}
		Logger::print("Webserv is shutting down...", NULL, INFO, SILENT);
		it = clients.begin();
		end = clients.end();
		while (!clients.empty())
		{
			Logger::print("Socket closed", NULL, SUCCESS, VERBOSE);
			delete (*it).sckt;
			delete (*it).req;
			it = clients.erase(it);
		}
	}
	catch(ServerSocket::ServerSocketException& e)
	{
		Logger::print(e.what(), NULL, ERROR, NORMAL);
	}
	Logger::print("Webserv Shutdown complete", NULL, SUCCESS, SILENT);
	std::cout << std::flush;
	return (0);
}
