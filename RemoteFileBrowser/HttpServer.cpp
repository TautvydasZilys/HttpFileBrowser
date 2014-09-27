#include "PrecompiledHeader.h"
#include "HttpServer.h"

using namespace std;
using namespace Utilities;

static const int kDataBufferSize = 4096;

void HttpServer::StartServiceClient(SOCKET incomingSocket, sockaddr_in clientAddress, HttpRequestExecutionHandler executionHandler)
{
	HttpServer serverInstance(incomingSocket, clientAddress, executionHandler);
	serverInstance.Run();
}

HttpServer::HttpServer(SOCKET incomingSocket, sockaddr_in clientAddress, HttpRequestExecutionHandler executionHandler) :
	m_ConnectionSocket(incomingSocket), m_ClientAddress(clientAddress), m_ReceivedData(nullptr), m_HasReportedUserAgent(false), m_ExecutionHandler(executionHandler)
{
}

void HttpServer::Run()
{
	char buffer[kDataBufferSize];	// Store on the stack, no need to use heap for such small buffer

	m_BytesReceived = 1;
	m_ReceivedData = buffer;

	while (m_BytesReceived > 0)
	{
		m_BytesReceived = recv(m_ConnectionSocket, buffer, kDataBufferSize, 0);

		if (m_BytesReceived > 0)
		{
			HandleRequest();
		}
		else if (m_BytesReceived < 0)
		{
			ReportConnectionDroppedError();
		}
	}

	closesocket(m_ConnectionSocket);
}

void HttpServer::HandleRequest()
{
	auto requestType = ParseRequest();

	// Only 'GET' request is supported
	if (requestType.length() < 3 || requestType[0] != 'G' && requestType[1] != 'E' && requestType[2] != 'T')
	{
		Logging::Log(L"Unknown request type: " + Encoding::Utf8ToUtf16(requestType));
		return;
	}

	// Get request looks like this:
	// GET <ActualRequestedData> <httpVersion>
	auto lastSpacePosition = requestType.find(' ', 5);

	// Check whether there's http version specified; if not - request is invalid
	if (lastSpacePosition == string::npos || lastSpacePosition == requestType.length() - 1)
	{
		return;	
	}

	// Extract and fix up requested path
	auto requestedPath = Encoding::DecodeUrl(requestType.substr(5, lastSpacePosition - 5));
	std::replace(begin(requestedPath), end(requestedPath), '/', '\\');

	auto httpVersion = requestType.substr(lastSpacePosition + 1);

	m_ExecutionHandler(m_ConnectionSocket, requestedPath, httpVersion);
}

std::string HttpServer::ParseRequest()
{
	int position = 0;

	// Parse request type

	int lineFeedPos = FindNextCharacter(position, '\r');
	string requestType(m_ReceivedData, lineFeedPos);

	// Parse rest of header only if we haven't reported user agent yet

	if (!m_HasReportedUserAgent)
	{
		ReportUserAgent(lineFeedPos + 2);
		m_HasReportedUserAgent = true;
	}

	return requestType;
}

void HttpServer::SendResponse(const string& response)
{
	auto sendResult = send(m_ConnectionSocket, response.c_str(), response.length(), 0);
	Assert(sendResult != SOCKET_ERROR);

	if (sendResult == SOCKET_ERROR)
	{
		Logging::Error(WSAGetLastError(), L"Failed to send response: ");
	}
}

int HttpServer::FindNextCharacter(int position, char character)
{
	while (position < m_BytesReceived - 1)
	{
		if (m_ReceivedData[position] == character)
		{
			return position;
		}

		position++;
	}

	return position;
}

void HttpServer::ReportUserAgent(int dataOffset)
{
	map<string, string> httpHeader;

	for (;;)
	{
		int semicolonPos = FindNextCharacter(dataOffset, ':');

		if (semicolonPos >= m_BytesReceived - 2)
		{
			break;
		}

		auto lineFeedPos = FindNextCharacter(semicolonPos + 2, '\r');

		string key(m_ReceivedData + dataOffset, semicolonPos - dataOffset);
		string value(m_ReceivedData + semicolonPos + 2, lineFeedPos - semicolonPos - 2);

		httpHeader.emplace(std::move(key), std::move(value));
		dataOffset = lineFeedPos + 2;
	}

	Logging::Log(L"Client user agent: " + Utilities::Encoding::Utf8ToUtf16(httpHeader["User-Agent"]));
}

void HttpServer::ReportConnectionDroppedError()
{
	const int bufferSize = 256;
	wchar_t msgBuffer[bufferSize];

	swprintf_s(msgBuffer, bufferSize, L"Connection from %d.%d.%d.%d dropped: ",
		m_ClientAddress.sin_addr.S_un.S_un_b.s_b1,
		m_ClientAddress.sin_addr.S_un.S_un_b.s_b2,
		m_ClientAddress.sin_addr.S_un.S_un_b.s_b3,
		m_ClientAddress.sin_addr.S_un.S_un_b.s_b4);

	Logging::Error(WSAGetLastError(), msgBuffer);
}