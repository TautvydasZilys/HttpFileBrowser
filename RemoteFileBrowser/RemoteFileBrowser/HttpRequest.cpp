#include "PrecompiledHeader.h"
#include "HttpRequest.h"

using namespace Http;

class RequestParser
{
private:
	static const int kBufferLength = 1280;
	char (&m_Buffer)[kBufferLength];
	int m_BufferPosition = 0;
	Request m_Request;
	int m_BytesReceived;
	int m_ContentLength = -1;

	inline int FindNextCharacter(char character, int position)
	{
		while (position < m_BytesReceived - 1)
		{
			if (m_Buffer[position] == character)
			{
				return position;
			}

			position++;
		}

		return position;
	}

	RequestParser(char (&buffer)[kBufferLength]) :
		m_Buffer(buffer)
	{
	}

	// <Verb> <Path> <HTTPVERSION>
	inline bool ParseFirstHeaderLine()
	{
		m_BufferPosition = FindNextCharacter(' ', m_BufferPosition) + 1;

		if (m_BufferPosition == m_BytesReceived)
			return false;

		if (strncmp(m_Buffer, "POST", 4) == 0)
		{
			m_Request.requestVerb = RequestVerb::POST;
		}
		else if (strncmp(m_Buffer, "GET", 3) == 0)
		{
			m_Request.requestVerb = RequestVerb::GET;
		}
		else
		{
			m_Buffer[m_BufferPosition - 1] = '\0';
			Utilities::Logging::Log(L"[ERROR] Unknown HTTP request verb: ", Utilities::Encoding::Utf8ToUtf16(m_Buffer));
			return false;
		}

		auto spacePosition = FindNextCharacter(' ', m_BufferPosition);
		
		if (spacePosition == m_BytesReceived)
			return false;

		m_Request.requestPath.assign(m_Buffer + m_BufferPosition, m_Buffer + spacePosition);
		m_BufferPosition = FindNextCharacter('\n', spacePosition + 1) + 1;

		return true;
	}

	inline bool ParseHeaderLine()
	{
		auto semicolonPosition = FindNextCharacter(':', m_BufferPosition);

		if (semicolonPosition > m_BytesReceived - 2)
			return false;

		auto newLinePosition = FindNextCharacter('\n', semicolonPosition + 1);

		if (newLinePosition - semicolonPosition < 3)
			return false;

		if (strncmp(m_Buffer + m_BufferPosition, "content-length", semicolonPosition - m_BufferPosition) == 0)
		{
			m_ContentLength = atoi(m_Buffer + semicolonPosition + 1);
			if (m_ContentLength < 1)
				return false;
		}
		else if (strncmp(m_Buffer + m_BufferPosition, "content-type", semicolonPosition - m_BufferPosition) == 0)
		{
			if (strncmp(m_Buffer + semicolonPosition + 2, "text/html", newLinePosition - semicolonPosition - 2) == 0)
			{
				m_Request.contentType = ContentType::HTML;
			}
			else if (strncmp(m_Buffer + semicolonPosition + 2, "application/json", newLinePosition - semicolonPosition - 2) == 0)
			{
				m_Request.contentType = ContentType::JSON;
			}
			else
			{
				m_Buffer[newLinePosition] = '\0';
				Utilities::Logging::Log(L"[ERROR] Unknown http request content type: ", Utilities::Encoding::Utf8ToUtf16(m_Buffer + semicolonPosition + 2));
				return false;
			}
		}

		m_BufferPosition = newLinePosition + 1;
		return true;
	}

	inline void ParseBody(SOCKET s)
	{
		m_Request.content.resize(m_ContentLength);

		auto bytesLeft = m_BytesReceived - m_BufferPosition;
		if (bytesLeft >= m_ContentLength)
		{
			memcpy(&m_Request.content[0], m_Buffer + m_BufferPosition, m_ContentLength);
			return;
		}

		memcpy(&m_Request.content[0], m_Buffer + m_BufferPosition, bytesLeft);
		bytesLeft = m_ContentLength - bytesLeft;

		do
		{
			auto bytesReceived = recv(s, &m_Request.content[m_ContentLength - bytesLeft], bytesLeft, 0);
			bytesLeft -= bytesReceived;
		}
		while (bytesLeft > 0);
	}

	void Parse(SOCKET s)
	{
		m_BytesReceived = recv(s, m_Buffer, kBufferLength, 0);
		
		if (m_BytesReceived < 1 || !ParseFirstHeaderLine())
		{
			goto fail;
		}

		while (m_Buffer[m_BufferPosition] != '\n')
		{
			if (!ParseHeaderLine())
			{
				goto fail;
			}
		}

		ParseBody(s);
		return;

	fail:
		m_Request = Request();
	}

public:
	static Request ReceiveAndParse(SOCKET s)
	{
		char buffer[kBufferLength];
		RequestParser parser(buffer);
		parser.Parse(s);

		return std::move(parser.m_Request);
	}
};



Request::Request() :
	requestVerb(RequestVerb::INVALID_REQUEST),
	contentType(ContentType::INVALID_CONTENT_TYPE)
{
}

Request::Request(SOCKET s)
{
	*this = RequestParser::ReceiveAndParse(s);
}

Request::Request(Request&& other) :
	requestVerb(other.requestVerb),
	contentType(other.contentType),
	requestPath(std::move(other.requestPath)),
	content(std::move(other.content))
{
}

Request::~Request()
{
}

Request& Request::operator= (Request&& other)
{
	requestVerb = other.requestVerb;
	contentType = other.contentType;
	requestPath = std::move(other.requestPath);
	content = std::move(other.content);

	return *this;
}

std::string Request::BuildHeaderString()
{
	using namespace std;

	stringstream header;

	switch (requestVerb)
	{
	case RequestVerb::GET:
		header << "GET ";
		break;

	case RequestVerb::POST:
		header << "POST ";
		break;

	default:
		Utilities::Logging::FatalError(ERROR_INVALID_DATA, L"Failed to build HTTP header string - unknown RequestVerb - ");
		break;
	}

	header << requestPath << " HTTP/1.1" << endl;
	header << "content-type: ";
	
	switch (contentType)
	{
	case ContentType::HTML:
		header << "text/html";
		break;

	case ContentType::JSON:
		header << "application/json";
		break;

	default:
		Utilities::Logging::FatalError(ERROR_INVALID_DATA, L"Failed to build HTTP header string - unknown contentType - ");
		break;
	}

	header << endl;
	header << "content-length: " << content.length() << endl;

	return header.str();
}