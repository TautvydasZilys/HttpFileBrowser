#include "PrecompiledHeader.h"
#include "ClientServerConnection.h"
#include "FileBrowserResponseHandler.h"
#include "HttpServer.h"
#include "RestCommunicator.h"
#include "TcpListener.h"

using namespace std;
using namespace Utilities;

// Send the server my unique ID
// Receive my ID in the server
// Listen for incoming clients
// Receive client IPs and whitelist them
void ClientServerConnection::Create(SOCKET connectionSocket, const string& hostname)
{
	static const string postUrl = "/api/RegisterConnection";
	static const string systemUniqueIdKey = "SystemUniqueId";
	const auto& systemUniqueIdValue = System::GetUniqueSystemId();

	// Send REST request

	RestCommunicator::Post(connectionSocket, hostname, postUrl, systemUniqueIdKey, systemUniqueIdValue);
	if (!RestCommunicator::ReceiveResponse(connectionSocket))
	{
		return;
	}

	// Find my port out

	sockaddr_in socketAddress;
	int socketAddressLength = sizeof(socketAddress);

	auto result = getsockname(connectionSocket, reinterpret_cast<sockaddr*>(&socketAddress), &socketAddressLength);
	Assert(result == ERROR_SUCCESS);
	
	// Start listening for connection

	TcpListener listener;
	listener.RunAsync(socketAddress.sin_addr.S_un.S_addr, socketAddress.sin_port, [](SOCKET incomingSocket, sockaddr_in clientAddress)
	{
		HttpServer::StartServiceClient(incomingSocket, clientAddress, &FileBrowserResponseHandler::ExecuteRequest);
	});

	// Receive client IPs

	for (;;)
	{
		unordered_map<string, string> clientInfo;

		if (!RestCommunicator::Receive(connectionSocket, clientInfo))	// Connection dropped
		{
			return;
		}
		
		static const string ipKey("ip");
		auto clientIp = clientInfo.find(ipKey);

		if (clientIp == clientInfo.end())	// Server sent us garbage
		{									// Drop connection from our side
			return;
		}

		int ipNumeric = 0;

		try
		{
			ipNumeric = stoi(clientIp->second);
		}
		catch (...)	// Invalid IP
		{
			return;
		}

		if (ipNumeric == 0)	// IP can't be 0
		{
			return;
		}

		listener.WhitelistIP(ipNumeric);
	}	
}
