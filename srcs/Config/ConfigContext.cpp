/*
** server_name
** port
** Le 1er serveur pour host:port est celui par défaut
** Pages d'erreurs par défaut
** Taille max du body client
** location
**    liste de methods activées
**    root
**    directory_listing
**    fichier par défaut si la requête est un dossier
**    tous les trucs cgi a voir plus tard
**    choisir le dossier où mettre les fichiers uploadés
*/

#include "ConfigContext.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include <algorithm>
#include <sstream>
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <list>
#include "Webserv.hpp"
#include "Methods.h"

/*
** ---------------------------------- CONSTRUCTOR --------------------------------------
*/

ConfigContext::ConfigContext()
	: _autoIndex(false)
{
	_directive_names.push_back("server_name");
	_directive_names.push_back("listen");
	_directive_names.push_back("root");
	_directive_names.push_back("location");
	_directive_names.push_back("autoindex");
	_directive_names.push_back("max_client_body_size");
	_directive_names.push_back("disable_methods");
	_directive_names.push_back("cgi-dir");
	_directive_names.push_back("cgi-ext");
	_directive_names.push_back("uploads");
}

ConfigContext::~ConfigContext()
{}

ConfigContext::ConfigContext(const ConfigContext& other)
{
	*this = other;
}

ConfigContext& ConfigContext::operator=(const ConfigContext& other)
{
	_params = other._params;
	_error_pages = other._error_pages;
	_autoIndex = other._autoIndex;
	_directive_names = other._directive_names;
	_childs = other._childs;
	_names = other._names;
	_acceptedMethods = other._acceptedMethods;
	_cgi_exts = other._cgi_exts;
	return *this;
}

ConfigContext::ConfigContext(const std::string& raw, const std::string& name, const ConfigContext* parent)
	: _autoIndex(false)
{
	// si name a une valeur, c'est un context de location, on interdit donc certaines directives
	if (parent != NULL)
	{
		_params = parent->_params;
		_error_pages = parent->_error_pages;
		_autoIndex = parent->_autoIndex;
		_acceptedMethods = parent->_acceptedMethods;
		_cgi_exts = parent->_cgi_exts;
		_names.push_back(name);
	}
	else
	{
		_directive_names.push_back("server_name");
		_directive_names.push_back("listen");
		_directive_names.push_back("location");
		for (std::list<IMethod*>::const_iterator it = g_webserv.methods.getRegistered().begin(); it != g_webserv.methods.getRegistered().end(); it++)
			_acceptedMethods.push_back(*it);
	}
	_directive_names.push_back("root");
	_directive_names.push_back("error_page");
	_directive_names.push_back("index");
	_directive_names.push_back("autoindex");
	_directive_names.push_back("max_client_body_size");
	_directive_names.push_back("disable_methods");
	_directive_names.push_back("cgi-dir");
	_directive_names.push_back("cgi-ext");
	_directive_names.push_back("uploads");


	for (size_t i = 0; i < raw.size();)
	{
		// On skip les whitepsaces en début de ligne
		i = raw.find_first_not_of(" \t", i);
		if (i == std::string::npos)
			break;

		// Si la ligne est vide ou que c'est un commentaire, on skip la ligne
		if (raw[i] == '\n' || raw[i] == '#')
		{
			i = raw.find('\n', i) + 1;
			continue;
		}

		// On récupère le nom de la directive, par exemple : "server_name" ou "root", etc.
		std::string directive_name = raw.substr(i, raw.find_first_of(" \t", i) - i);
		if (std::find(_directive_names.begin(), _directive_names.end(), directive_name) == _directive_names.end())
			throw std::invalid_argument("Unknown directive name in config");
		if (_params.find(directive_name) != _params.end())
			_params.erase(_params.find(directive_name));
		i += directive_name.size();
		i = raw.find_first_not_of(" \t", i);

		if (directive_name == "location")
		{
			// On récupère le chemin de la location (juste après le nom de la directive)
			std::string location_name = raw.substr(i, raw.find_first_of(" \t\n", i) - i);
			i += location_name.size();
			i = raw.find_first_not_of(" \t", i);

			// Si il n'y a pas d'accolate ouvrante, la config est invalide
			if (raw[i] != '{')
				throw std::invalid_argument("Bad curly braces in config");
			i = raw.find_first_not_of(" \t", i + 1);

			// Si il n'y a pas de retour à la ligne après les espaces, la config est invalide
			if (raw[i] != '\n')
				throw std::invalid_argument("Bad new line in config");

			// On récupère le contenu du fichier qui concerne la location
			std::string location_raw = raw.substr(i, find_closing_bracket(raw, i) - i - 2);
			_childs.push_back(ConfigContext(location_raw, location_name, this));
			i += location_raw.size() + 2;
			if (raw.find('\n', i) == std::string::npos)
				break;
			i = raw.find('\n', i) + 1;
			continue;
		}
		else if (directive_name == "server_name")
			parse_server_name(raw, i);
		else if (directive_name == "error_page")
			parse_error_page(raw, i);
		else if (directive_name == "autoindex")
			parse_autoindex(raw, i);
		else if (directive_name == "disable_methods")
			parse_methods(raw, i);
		else if (directive_name == "cgi-ext")
			parse_cgi_ext(raw, i);
		else
		{
			std::string directive_value = raw.substr(i, raw.find('\n', i) - i);
			if (directive_value.size() == 0)
				throw std::invalid_argument("Empty directive value in config");
			_params.insert(std::make_pair(directive_name, ft::split(directive_value, " \t")));
		}
		if (raw.find('\n', i) == std::string::npos)
			break;
		i = raw.find('\n', i) + 1;
	}
	if ((std::find(_directive_names.begin(), _directive_names.end(), "listen") != _directive_names.end() && _params.find("listen") == _params.end()))
		throw std::invalid_argument("No listen in config");
	set_uploads_default();
	set_root_default();
	set_max_body_size_default();
	set_cgi_dir_default();
}

void ConfigContext::parse_cgi_ext(const std::string& raw, const int i)
{
	std::string directive_value = raw.substr(i, raw.find('\n', i) - i);
	if (directive_value.size() == 0)
		throw std::invalid_argument("Empty directive value in config");
	std::list<std::string> words = ft::split(directive_value, " \t\n");
	if (words.size() & 1)
		throw std::invalid_argument("No cgi path was provided for cgi-ext in config");
	bool extension_arg = true;
	std::string extension;
	for (std::list<std::string>::iterator it = words.begin(); it != words.end(); it++)
	{
		if (extension_arg)
		{
			if ((*it)[0] != '.')
				throw std::invalid_argument("Extension doesn't start with a dot for cgi-ext in config");
			extension = *it;
		}
		else
			_cgi_exts.insert(std::make_pair(extension, *it));
		extension_arg = !extension_arg;
	}
	for (std::map<std::string, std::string>::iterator it = _cgi_exts.begin(); it != _cgi_exts.end(); it++)
	{
		it->second = ft::simplify_path(it->second);
		if (it->second != "/" && it->second != "")
			it->second.erase(--(it->second.end()));
		if (!ft::is_executable(it->second.c_str()))
			throw std::invalid_argument("cgi-ext parameter is not an executable in config");
	}
}

void ConfigContext::set_root_default()
{
	if (_params.find("root") == _params.end())
		_params["root"].push_back("./");
	else
	{
		_params["root"].front() = ft::simplify_path(_params["root"].front());
		if (_params["root"].front() != "/" && _params["root"].front() != "")
			_params["root"].front().erase(--_params["root"].front().end());
		if (!ft::is_directory(_params["root"].front().c_str()))
			throw std::invalid_argument("Root parameter is not a folder in config");
	}
}

void ConfigContext::set_cgi_dir_default()
{
	if (_params.find("cgi-dir") == _params.end())
		_params["cgi-dir"].push_back("/cgi-bin/");
	else
	{
		_params["cgi-dir"].front() = ft::simplify_path(_params["cgi-dir"].front());
		if (_params["cgi-dir"].front() != "/" && !_params["cgi-dir"].front().empty())
			_params["cgi-dir"].front().erase(--_params["cgi-dir"].front().end());
		if (!ft::is_directory(_params["cgi-dir"].front().c_str()))
			throw std::invalid_argument("cgi-dir parameter is not a folder in config");
	}
}

void ConfigContext::parse_methods(const std::string& raw, const int i)
{
	std::string directive_value = raw.substr(i, raw.find('\n', i) - i);
	if (directive_value.size() == 0)
		throw std::invalid_argument("Empty directive value in config");
	std::list<std::string> words = ft::split(directive_value, " \t\n");
	_acceptedMethods.assign(g_webserv.methods.getRegistered().begin(), g_webserv.methods.getRegistered().end());
	if (words.front() == "none")
		return;
	for (std::list<std::string>::const_iterator it = words.begin(); it != words.end(); it++)
	{
		if (g_webserv.methods.getByType(*it) == NULL)
			throw std::invalid_argument("Bad method name in method field in config.");
		_acceptedMethods.remove(g_webserv.methods.getByType(*it));
	}
}

void ConfigContext::parse_autoindex(const std::string& raw, const int i)
{
	// On récupère la valeur de la directive : exemple "400 404 405 error.html"
	std::string directive_value = raw.substr(i, raw.find('\n', i) - i);
	if (directive_value.size() == 0)
		throw std::invalid_argument("Empty directive value in config");

	std::list<std::string> words = ft::split(directive_value, " \t\n");
	std::string val = words.front();

	if (val != "on" && val != "off")
		throw std::invalid_argument("Bad value for autoindex in config");
	_autoIndex = (val == "on" ? true : false);
}

void ConfigContext::parse_error_page(const std::string& raw, const int i)
{
	// On récupère la valeur de la directive : exemple "400 404 405 error.html"
	std::string directive_value = raw.substr(i, raw.find('\n', i) - i);
	if (directive_value.size() == 0)
		throw std::invalid_argument("Empty directive value in config");

	std::list<std::string> words = ft::split(directive_value, " \t\n");
	std::string page = words.back();

	for (std::list<std::string>::const_iterator it = words.begin(); it != --words.end(); it++)
		_error_pages.insert(std::make_pair(ft::toInt(*it), page));
}

void ConfigContext::parse_server_name(const std::string& raw, const int i)
{
	// On récupère la valeur de la directive : exemple "localhost www.localhost.com pouetpouet.fr"
	std::string directive_value = raw.substr(i, raw.find('\n', i) - i);
	if (directive_value.size() == 0)
		throw std::invalid_argument("Empty directive value in config");

	for (size_t j = 0; j < directive_value.size();)
	{
		std::string name = directive_value.substr(j, directive_value.find_first_of(" \t\n", j) - j);
		j = directive_value.find_first_of(" \t\n", j);
		_names.push_back(name);

		// Si on est arrivé au dernier mot de la directive, on arrête
		if (j >= directive_value.size() || directive_value[j] == '\n')
			break;
		j = directive_value.find_first_not_of(" \t\n", j);
		if (j >= directive_value.size())
			break;
	}
}

void ConfigContext::set_uploads_default()
{
	if (_params.find("uploads") == _params.end())
		_params["uploads"].push_back("./");
	else
	{
		_params["uploads"].front() = ft::simplify_path(_params["uploads"].front());
		if (_params["uploads"].front() != "/" && _params["uploads"].front() != "")
			_params["uploads"].front().erase(--_params["uploads"].front().end());
		if (!ft::is_directory(_params["uploads"].front().c_str()))
			throw std::invalid_argument("Uploads parameter is not a folder in config");
	}
}

void ConfigContext::set_max_body_size_default()
{
	if (_params.find("max_client_body_size") == _params.end())
		_params["max_client_body_size"].push_back("8000");
	else
	{
		if (!ft::is_integer<int>(_params["max_client_body_size"].front()))
			throw std::invalid_argument("max_client_body_size is not an integer in config.");
		if (_params["max_client_body_size"].front()[0] == '-')
			throw std::invalid_argument("Negative max_client_body_size in config.");
	}
}

/*
** ---------------------------------- ACCESSOR --------------------------------------
*/

const std::list<const IMethod*>& ConfigContext::getAllowedMethods() const
{
	return _acceptedMethods;
}

bool ConfigContext::hasAutoIndex() const
{
	return _autoIndex;
}

bool ConfigContext::hasAutoIndexPath(const std::string& path) const
{
	for (std::list<ConfigContext>::const_iterator it = _childs.begin(); it != _childs.end(); it++)
		if (it->getNames().front() == path.substr(0, std::min(path.size(), it->getNames().front().size())))
			return it->hasAutoIndex();
	return hasAutoIndex();
}

const std::list<std::string>& ConfigContext::getParam(const std::string& name) const
{
	if (_params.find(name) == _params.end())
		throw std::invalid_argument("Parameter not found in config");
	return _params.find(name)->second;
}

const std::list<std::string>& ConfigContext::getParamPath(const std::string& name, const std::string& path) const
{
	for (std::list<ConfigContext>::const_iterator it = _childs.begin(); it != _childs.end(); it++)
	{
		if (it->getNames().front() == path.substr(0, std::min(path.size(), it->getNames().front().size())))
			return it->getParam(name);
	}
	return getParam(name);
}

const std::list<const IMethod*>& ConfigContext::getAllowedMethodsPath(const std::string& path) const
{
	for (std::list<ConfigContext>::const_iterator it = _childs.begin(); it != _childs.end(); it++)
	{
		if (it->getNames().front() == path.substr(0, std::min(path.size(), it->getNames().front().size())))
			return it->getAllowedMethods();
	}
	return getAllowedMethods();
}

std::string ConfigContext::rootPath(const std::string& path, int& base_depth) const
{
	std::string res;

	for (std::list<ConfigContext>::const_iterator it = _childs.begin(); it != _childs.end(); it++)
	{
		if (it->getNames().front() == path.substr(0, std::min(path.size(), it->getNames().front().size())))
		{
			res = (it->getParam("root").front()[0] == '/' ? "/" : "");

			std::list<std::string> splitted = ft::split(it->getParam("root").front(), "/");
			base_depth = splitted.size();
			std::list<std::string> splitted_path = ft::split(path.substr(it->getNames().front().size()), "/");

			for (std::list<std::string>::iterator it = splitted.begin(); it != splitted.end(); it++)
				res += *it + "/";
			for (std::list<std::string>::iterator it2 = splitted_path.begin(); it2 != splitted_path.end(); it2++)
				res += *it2 + "/";
			if (res[res.size() - 1] == '/' && res != "/")
				res.erase(--res.end()); // Enlève le / à la fin
			return res;
		}
	}
	res = (getParam("root").front()[0] == '/' ? "/" : "");

	std::list<std::string> splitted = ft::split(getParam("root").front(), "/");
	base_depth = splitted.size();
	std::list<std::string> splitted_path = ft::split(path, "/");

	for (std::list<std::string>::iterator it = splitted.begin(); it != splitted.end(); it++)
		res += *it + "/";
	for (std::list<std::string>::iterator it2 = splitted_path.begin(); it2 != splitted_path.end(); it2++)
		res += *it2 + "/";
	if (res[res.size() - 1] == '/' && res != "/")
		res.erase(--res.end()); // Enlève le / à la fin
	return res;
}

std::list<ConfigContext>& ConfigContext::getChilds()
{
	return _childs;
}

std::string ConfigContext::getErrorPage(int code) const
{
	if (_error_pages.find(code) != _error_pages.end())
	{
		try
		{
			std::ifstream file(_error_pages.find(code)->second.c_str(), std::ifstream::in);
			std::string content((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
			return content;
		}
		catch (std::exception& e)
		{}
	}
	std::stringstream ss;
	ss << code;
	std::string code_str = ss.str();
	return
"<html>\r\n\
	<head><title>" + code_str + " " + ft::getErrorMessage(code) + "</title></head>\r\n\
	<body>\r\n\
		<center><h1>" + code_str + " " + ft::getErrorMessage(code) + "</h1></center>\r\n\
		<hr><center>Webserv 1.0.0</center>\r\n\
	</body>\r\n\
</html>";
}

const std::map<std::string, std::string>& ConfigContext::getCGIExtensions() const
{
	return _cgi_exts;
}

const std::map<std::string, std::string>& ConfigContext::getCGIExtensionsPath(const std::string& path) const
{
	for (std::list<ConfigContext>::const_iterator it = _childs.begin(); it != _childs.end(); it++)
		if (it->getNames().front() == path.substr(0, std::min(path.size(), it->getNames().front().size())))
			return it->getCGIExtensions();

	return getCGIExtensions();
}

std::string ConfigContext::getErrorPagePath(int code, const std::string& path) const
{
	for (std::list<ConfigContext>::const_iterator it = _childs.begin(); it != _childs.end(); it++)
	{
		if (it->getNames().front() == path.substr(0, std::min(path.size(), it->getNames().front().size())))
		{
			if (it->getErrorPages().find(code) != it->getErrorPages().end())
				return it->getErrorPage(code);
		}
	}
	return getErrorPage(code);
}

std::string ConfigContext::findFileWithRoot(const std::string& name) const
{
	const std::list<std::string>& roots = getParam("root");
	for (std::list<std::string>::const_iterator it = roots.begin(); it != roots.end(); it++)
	{
		std::ifstream file((*it + "/" + name).c_str());
		if (file.good())
			return *it + "/" + name.c_str();
	}
	throw std::invalid_argument("File doesn't exist.");
}

const std::map<std::string, std::list<std::string> >& ConfigContext::getParams() const
{
	return _params;
}

const std::map<int, std::string>& ConfigContext::getErrorPages() const
{
	return _error_pages;
}

const std::list<std::string>& ConfigContext::getNames() const
{
	return _names;
}

/*
** --------------------------------------- METHODS ---------------------------------------
*/

size_t ConfigContext::find_closing_bracket(const std::string& str, size_t start) const
{
	size_t nesting = 1;

	for (size_t i = start; i < str.size(); i++)
	{
		if (str[i] == '{')
			nesting++;
		if (str[i] == '}')
			nesting--;
		if (nesting == 0)
			return i;
	}
	return size_t(-1);
}

std::string ConfigContext::toString() const
{
	std::string str;

	str += "Names:\n";
	for (std::list<std::string>::const_iterator it = _names.begin(); it != _names.end(); it++)
		str += "  " + *it + "\n";

	if (!_params.empty())
	{
		str += "Params:\n";
		for (std::map<std::string, std::list<std::string> >::const_iterator it = _params.begin(); it != _params.end(); it++)
		{
			str += "  " + it->first + "\n";
			for (std::list<std::string>::const_iterator it2 = it->second.begin(); it2 != it->second.end(); it2++)
				str += "    " + *it2 + "\n";
		}
	}

	if (!_childs.empty())
	{
		str += "Childs:\n";
		for (std::list<ConfigContext>::const_iterator it = _childs.begin(); it != _childs.end(); it++)
			str += it->toString();
	}
	return str;
}
