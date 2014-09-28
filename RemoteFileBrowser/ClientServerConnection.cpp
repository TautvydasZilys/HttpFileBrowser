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
// Receive client IPs
// Listen for incoming clients
void ClientServerConnection::Create(SOCKET connectionSocket)
{
	static const string postUrl = "/registerConnection";
	static const string systemUniqueIdKey = "SystemUniqueId";
	const auto& systemUniqueIdValue = System::GetUniqueSystemId();

	// Send REST request

	RestCommunicator::Post(connectionSocket, postUrl, systemUniqueIdKey, systemUniqueIdValue);

	// Find my port out

	sockaddr_in socketAddress;
	int socketAddressLength = sizeof(socketAddress);

	auto result = getsockname(connectionSocket, reinterpret_cast<sockaddr*>(&socketAddress), &socketAddressLength);
	Assert(result == ERROR_SUCCESS);

	auto port = htons(socketAddress.sin_port);

	// Receive client IPs

	for (;;)
	{
		auto clientInfo = RestCommunicator::Receive(connectionSocket);
		
		static const string ipKey("ip");
		auto clientIp = clientInfo.find(ipKey);

		if (clientIp == clientInfo.end())	// Either connection terminated, or server sent us garbage
		{									// Drop connection from our side either way
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

		TcpListener::Run(port, ipNumeric, [](SOCKET incomingSocket, sockaddr_in clientAddress)
		{
			HttpServer::StartServiceClient(incomingSocket, clientAddress, &FileBrowserResponseHandler::ExecuteRequest);
		});
	}	
}
