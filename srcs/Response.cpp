#include "Response.hpp"
#include <sstream>

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

Response::Response()
	: _code(0)
{}

Response::~Response()
{}

Response::Response(const Response& other)
{
	*this = other;
}

Response::Response(int code, const std::string& message)
	: _code(code), _message(message)
{}

Response& Response::operator=(const Response& other)
{
	_code = other._code;
	_message = other._message;
	_headers = other._headers;
	_body = other._body;
	return *this;
}

/*
** ------------------------------- METHODS --------------------------------
*/

void Response::addHeader(const std::string& header_name, const std::string& header_value)
{
	_headers[header_name] = header_value;
}

void Response::removeHeader(const std::string& header_name)
{
	_headers.erase(header_name);
}

std::string Response::getResponseText()
{
	std::stringstream ss;
	ss << _code;

	std::string code_str = ss.str();

	std::string str = "HTTP/1.1 " + code_str + " " + _message + "\r\n";

	for (std::map<std::string, std::string>::iterator it = _headers.begin(); it != _headers.end(); it++)
	{
		str += it->first + ": " + it->second + "\r\n";
	}
	str += "\r\n";

	if (_body != "")
		str += _body;
	if (_body == "" && _code >= 300)
	{
		str = str +
				"<html>\r\n" +
				"<head><title>" + code_str + " " + _message + "</title></head>\r\n" +
				"<body>\r\n" +
				"<center><h1>" + code_str + " " + _message + "</h1></center>\r\n" +
				"<hr><center>Webserv 1.0.0</center>\r\n" +
				"</body>\r\n" +
				"</html>\r\n";
	}
	return str;
}

/*
** ------------------------------- ACCESSORS --------------------------------
*/

void Response::setCode(int code)
{
	_code = code;
}

void Response::setMessage(const std::string& message)
{
	_message = message;
}

void Response::setBody(const std::string& body)
{
	_body = body;
}
