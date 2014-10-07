#pragma once

namespace Http
{
	enum class RequestVerb
	{
		INVALID_REQUEST = -1,
		GET,
		POST
	};

	enum class ContentType
	{
		INVALID_CONTENT_TYPE,
		HTML,
		JSON
	};

	class Request
	{
	public:
		RequestVerb requestVerb;
		ContentType contentType;
		std::string requestPath;
		std::string content;

		Request();
		Request(SOCKET s);
		Request(Request&& other);
		~Request();

		Request& operator=(Request&& other);

		Request(const Request&) = delete;
		Request& operator=(const Request&) = delete;

		std::string BuildHeaderString();
	};
}