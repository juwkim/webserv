#include "http.hpp"

/* ************************************************************************** */
// Orthodox Canonical Form
/* ************************************************************************** */

Http::~Http(void)
{
}

Http::Http(const Conf &conf):
	conf_(conf)
{
}

/* ************************************************************************** */
// Main
/* ************************************************************************** */

#include <iostream>
void Http::Execute()
{
	// check http version, only support HTTP/1.1
	if (request_.version().compare("HTTP/1.1") != 0)
	{
		this->GenerateError(kHttpVersionNotSupported);
		response_.set_done(true);
		return;
	}

	// check if url is valid
	const std::string url = conf_.GetUrl(request_.uri());
	if (url.empty())
	{
		this->GenerateError(kFound);
		response_.add_header("Location", "/");
		response_.set_done(true);
		return;
	}

	// get location
	const int location_idx = conf_.GetLocationIdx(url);
	if (location_idx == -1)
	{
		this->GenerateError(kNotFound);
		response_.set_done(true);
		return;
	}
	const location_t location = conf_.GetLocation(location_idx);

	// If return code is non-zero, it means that the request is redirected.
	if (location.ret.code != 0)
	{
		this->GenerateError(kFound);
		switch(location.ret.code)
		{
			case kMovedPermanently:
			case kFound:
			case kSeeOther:
			case kTemporaryRedirect:
			case kPermanentRedirect:
				response_.add_header("Location", location.ret.url);
				break;
			default:
				response_.body().str(location.ret.text);
				break;
		}
		response_.set_done(true);
		return;
	}

	std::cout << "Request Recived Method[" << request_.method() <<
				"] uri[" << request_.uri() <<
				"] url[" << url <<
				"] path[" << conf_.GetPath(url) <<
				"]" << std::endl;

	// check limit_except.methods
	const std::vector<std::string> allowed_methods = location.limit_except.methods;
	if (std::find(allowed_methods.begin(), allowed_methods.end(), request_.method()) == allowed_methods.end())
	{
		this->GenerateError(kMethodNotAllowed);
		response_.set_done(true);
		return;
	}

	// if (std::find(allowed_ips.begin(), allowed_ips.end(), request_.client_ip()) == allowed_ips.end())
	// check limit_except.allows
	const std::vector<std::string> allowed_ips = location.limit_except.allows;
	if (std::find(allowed_ips.begin(), allowed_ips.end(), "all") == allowed_ips.end())
	{
		this->GenerateError(kForbidden);
		response_.set_done(true);
		return;
	}

	// check limit_except.denys
	// const std::vector<std::string> denied_ips = location.limit_except.denys;
	// if (std::find(denied_ips.begin(), denied_ips.end(), request_.client_ip()) != denied_ips.end())
	// {
	// 	this->GenerateError(kForbidden);
	// 	response_.set_done(true);
	// 	return;
	// }

	// execute method
	HttpStatus status;
	if (request_.method().compare("GET") == 0)
		status = this->Get(location, url);
	else if (request_.method().compare("POST") == 0)
		status = this->Post(location);
	else if (request_.method().compare("DELETE") == 0)
		status = this->Delete(url);
	else
		status = kMethodNotAllowed;
	// if error
	if (200 <= status && status <= 299)
		return ;
	this->GenerateError(status);
	response_.set_done(true);
}


void Http::Do(std::stringstream& in, std::stringstream& out)
{
	if (response_.done())
	{
		response_ >> out;
		request_.Clear();
		response_.Clear();
		return ;
	}
	request_ << in;
	if (request_.step() == kParseDone)
		response_.set_done(true);
	if (request_.step() == kParseFail)
	{
		this->GenerateError(kBadRequest);
		response_.set_done(true);
	}
	if (request_.step() == 	kParseBodyStart)
	{
		this->Execute();
		request_.set_step(kParseBody);
	}
	else if (request_.step() == kParseChunkStart)
	{
		this->Execute();
		request_.set_step(kParseChunkLen);
	}
	else if (request_.step() == kParseBody || request_.step() == kParseChunkLen)
	{
		std::cout << "### " << request_.body().rdbuf() << std::endl;
	}
}