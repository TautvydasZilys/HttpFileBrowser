#include "PrecompiledHeader.h"
#include "HttpServer.h"
#include "Utilities.h"

using namespace std;

static const int kDataBufferSize = 4096;

void HttpServer::StartServiceClient(SOCKET incomingSocket, sockaddr_in clientAddress, HttpRequestExecutionHandler executionHandler)
{
	HttpServer serverInstance(incomingSocket, clientAddress, executionHandler);
	serverInstance.Run();
}

HttpServer::HttpServer(SOCKET incomingSocket, sockaddr_in clientAddress, HttpRequestExecutionHandler executionHandler) :
	m_ConnectionSocket(incomingSocket), m_ClientAddress(clientAddress), m_ExecutionHandler(executionHandler)
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
		Utilities::Log(L"Unknown request type: " + Utilities::Utf8ToUtf16(requestType));
		return;
	}

	auto lastSpacePosition = requestType.find(' ', 5);

	// Check whether there's http version specified; if not - request is invalid
	if (lastSpacePosition == string::npos || lastSpacePosition == requestType.length() - 1)
	{
		return;	
	}

	auto path = requestType.substr(5, lastSpacePosition - 5);
	auto httpVersion = requestType.substr(lastSpacePosition + 1);

	auto response = m_ExecutionHandler(path, httpVersion);
	SendResponse(response);
}

std::string HttpServer::ParseRequest()
{
	int position = 0;

	// Parse request type

	int lineFeedPos = FindNextCharacter(position, '\r');
	string requestType(m_ReceivedData, lineFeedPos);

	// Parse rest of header

	map<string, string> httpHeader;
	position = lineFeedPos + 2;

	for (;;)
	{
		int semicolonPos = FindNextCharacter(position, ':');

		if (semicolonPos >= m_BytesReceived - 2)
		{
			break;
		}

		lineFeedPos = FindNextCharacter(semicolonPos + 2, '\r');

		string key(m_ReceivedData + position, semicolonPos - position);
		string value(m_ReceivedData + semicolonPos + 2, lineFeedPos - semicolonPos - 2);

		httpHeader.emplace(std::move(key), std::move(value));
		position = lineFeedPos + 2;
	}

	auto userAgent = httpHeader["User-Agent"];
	Utilities::Log(L"Client user agent: " + Utilities::Utf8ToUtf16(userAgent));

	return requestType;
}

void HttpServer::SendResponse(const string& response)
{
	auto sendResult = send(m_ConnectionSocket, response.c_str(), response.length(), 0);
	Assert(sendResult != SOCKET_ERROR);

	if (sendResult == SOCKET_ERROR)
	{
		Utilities::Error(WSAGetLastError(), L"Failed to send response: ");
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

void HttpServer::ReportConnectionDroppedError()
{
	const int bufferSize = 256;
	wchar_t msgBuffer[bufferSize];

	swprintf_s(msgBuffer, bufferSize, L"Connection from %d.%d.%d.%d dropped: ",
		m_ClientAddress.sin_addr.S_un.S_un_b.s_b1,
		m_ClientAddress.sin_addr.S_un.S_un_b.s_b2,
		m_ClientAddress.sin_addr.S_un.S_un_b.s_b3,
		m_ClientAddress.sin_addr.S_un.S_un_b.s_b4);

	Utilities::Error(WSAGetLastError(), msgBuffer);
}