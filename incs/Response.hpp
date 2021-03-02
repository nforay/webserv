#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <map>

class Response
{
	private:
		int _code;
		std::string _message;
		std::map<std::string, std::string> _headers;
		std::string _body;

	public:
		Response();
		Response(const Response& other);
		Response(int code, const std::string& message);
		virtual ~Response();
		Response& operator=(const Response& other);

		void setCode(int code);
		void setMessage(const std::string& message);

		void addHeader(const std::string& header_name, const std::string& header_value);
		void removeHeader(const std::string& header_name);

		void setBody(const std::string& body);

		std::string getResponseText();
};

#endif