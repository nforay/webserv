#ifndef IMETHOD_HPP
#define IMETHOD_HPP

#include <string>
#include "Response.hpp"
#include "ServerSocket.hpp"

class Request;

class IMethod
{
	public:
		virtual ~IMethod() {};
		virtual std::string getType() const = 0;
		virtual IMethod* clone() const = 0;
		virtual bool allowAbsolutePath() const = 0;
		virtual bool allowCompleteURL() const = 0;
		virtual bool allowAuthorityURI() const = 0;
		virtual bool allowAsteriskURI() const = 0;

		// From https://developer.mozilla.org/en-US/docs/Web/HTTP/Methods
		virtual bool requestHasBody() const = 0;
		virtual bool successfulResponseHasBody() const = 0;
		virtual bool isSafe() const = 0;
		virtual bool isIdempotent() const = 0;
		virtual bool isCacheable() const = 0;
		virtual bool isAllowedInHTMLForms() const = 0;

		virtual Response process(const Request& request, const ConfigContext& config, const ServerSocket& socket) = 0;
};

#endif
