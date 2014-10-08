#include "PrecompiledHeader.h"
#include "ClientServerConnection.h"
#include "FileBrowserResponseHandler.h"
#include "Http\Server.h"
#include "Http\RestCommunicator.h"
#include "Tcp\Listener.h"

using namespace std;
using namespace Utilities;

// Send the server my unique ID
// Receive my ID in the server
// Listen for incoming clients
// Receive client IPs and whitelist them
void ClientServerConnection::Create(SOCKET connectionSocket, string&& hostname)
{
	static const string systemUniqueIdKey = "SystemUniqueId";
	const auto& systemUniqueIdValue = System::GetUniqueSystemId();

	// Send REST request

	Http::RestCommunicator::Post(connectionSocket, std::move(hostname), "/api/RegisterConnection", systemUniqueIdKey, systemUniqueIdValue);
	if (!Http::RestCommunicator::ReceiveResponse(connectionSocket))
	{
		Utilities::Logging::Log("[ERROR] Server didn't accept system unique ID.");
		return;
	}

	// Find my port out

	sockaddr_in socketAddress;
	int socketAddressLength = sizeof(socketAddress);

	auto result = getsockname(connectionSocket, reinterpret_cast<sockaddr*>(&socketAddress), &socketAddressLength);
	Assert(result == ERROR_SUCCESS);
	
	// Start listening for connection

	Tcp::Listener listener;
	listener.RunAsync(socketAddress.sin_addr.S_un.S_addr, socketAddress.sin_port, [](SOCKET incomingSocket, sockaddr_in clientAddress)
	{
		Http::Server::StartServiceClient(incomingSocket, clientAddress, &FileBrowserResponseHandler::ExecuteRequest);
	});

	// Receive client IPs

	for (;;)
	{
		unordered_map<string, string> clientInfo;

		if (!Http::RestCommunicator::ReceivePost(connectionSocket, clientInfo))	// Connection dropped
		{
			return;
		}
		
		static const string ipKey("ip");
		auto clientIp = clientInfo.find(ipKey);

		if (clientIp == clientInfo.end())	// Server sent us garbage
		{									// Drop connection from our side
			return;
		}

		UINT ipNumeric = inet_addr(clientIp->second.c_str());

		if (ipNumeric == 0 || ipNumeric == INADDR_NONE)	// IP can't be 0
		{
			return;
		}

		listener.WhitelistIP(ipNumeric);
	}	
}
