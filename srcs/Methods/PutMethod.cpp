#include <fstream>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "PutMethod.hpp"
#include "Request.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include "URL.hpp"
#include "Webserv.hpp"

PutMethod::PutMethod() {}
PutMethod::PutMethod(const PutMethod&) {}
PutMethod::~PutMethod() {}
PutMethod& PutMethod::operator=(const PutMethod&) { return *this; }

std::string PutMethod::getType() const { return "PUT"; }

IMethod* PutMethod::clone() const { return new PutMethod(*this); }

bool PutMethod::allowAbsolutePath() const { return true; }
bool PutMethod::allowCompleteURL() const { return true; }
bool PutMethod::allowAuthorityURI() const { return false; }
bool PutMethod::allowAsteriskURI() const { return false; }

bool PutMethod::requestHasBody() const { return true; }
bool PutMethod::successfulResponseHasBody() const { return false; }
bool PutMethod::isSafe() const { return false; }
bool PutMethod::isIdempotent() const { return true; }
bool PutMethod::isCacheable() const { return false; }
bool PutMethod::isAllowedInHTMLForms() const { return false; }

Response PutMethod::process(const Request& request, const ConfigContext& config, const ServerSocket&)
{
	URL		url(request._url._path);
	int		base_depth = 0;

	std::string realPath = config.rootPath(url._path, base_depth);
	try
	{
		realPath = ft::simplify_path(realPath, true, base_depth);
	}
	catch (std::exception& e)
	{
		return Response(404, url._path);
	}

	Response response(200, url._path);
	if (realPath.rfind('/') != config.getParam("root").front().size()) // if no ext or target is subfoler inside uploads
		return Response(415, url._path);

	if (realPath.rfind('.') != std::string::npos)
	{
		std::string extension = ft::get_extension(realPath);
		if (!config.can_be_uploaded(extension) || !g_webserv.file_formatname->GetNode(extension.substr(1))
		|| request.getHeaderValue("Content-Type") != g_webserv.file_formatname->Lookup(extension.substr(1)))
			return Response(415, url._path);
	}
	std::fstream file;
	struct stat	file_stats;
	if (lstat(realPath.c_str(), &file_stats) < 0)
	{
		file.open(realPath.c_str(), std::fstream::in | std::fstream::out | std::fstream::trunc);
		response.setCode(201);
		response.addHeader("Location", realPath.substr(realPath.find('/')));
	}
	else if (!S_ISREG(file_stats.st_mode))
		return Response(500, url._path);
	else
		file.open(realPath.c_str(), std::fstream::in | std::fstream::out | std::fstream::trunc);
	if (!file.good() || !file.is_open())
		return Response(500, url._path);
	response.addHeader("Content-Location", realPath.substr(realPath.find('/')));
	file << request._body;
	file.close();
	return (response);
}
