#include "PrecompiledHeader.h"
#include "Server.h"

using namespace std;
using namespace Http;
using namespace Utilities;

static const int kDataBufferSize = 4096;

void Server::StartServiceClient(SOCKET incomingSocket, sockaddr_in6 clientAddress, HttpRequestExecutionHandler executionHandler)
{
	Server serverInstance(incomingSocket, clientAddress, executionHandler);
	serverInstance.Run();
}

Server::Server(SOCKET incomingSocket, sockaddr_in6 clientAddress, HttpRequestExecutionHandler executionHandler) :
	m_ConnectionSocket(incomingSocket), m_ClientAddress(clientAddress), m_ReceivedData(nullptr), m_HasReportedUserAgent(false), m_ExecutionHandler(executionHandler)
{
}

void Server::Run()
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

void Server::HandleRequest()
{
	auto requestType = ParseRequest();

	// Only 'GET' request is supported
	if (requestType.length() < 3 || requestType[0] != 'G' && requestType[1] != 'E' && requestType[2] != 'T')
	{
		Logging::Log("Unknown request type: ", requestType);
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

std::string Server::ParseRequest()
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

void Server::SendResponse(const string& response)
{
	Assert(response.length() < std::numeric_limits<int>::max());
	auto sendResult = send(m_ConnectionSocket, response.c_str(), static_cast<int>(response.length()), 0);
	Assert(sendResult != SOCKET_ERROR);

	if (sendResult == SOCKET_ERROR)
	{
		Logging::Error(WSAGetLastError(), "Failed to send response: ");
	}
}

int Server::FindNextCharacter(int position, char character)
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

void Server::ReportUserAgent(int dataOffset)
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

	Logging::Log("Client user agent: ", httpHeader["User-Agent"]);
}

void Server::ReportConnectionDroppedError()
{
	const int bufferSize = 64;
	char msgBuffer[bufferSize];

	auto msgLength = Utilities::Encoding::IpToString(AF_INET6, &m_ClientAddress.sin6_addr, msgBuffer);
	Assert(msgLength != 0);

	Logging::Error(WSAGetLastError(), "Connection from ", msgBuffer, " dropped: ");
}