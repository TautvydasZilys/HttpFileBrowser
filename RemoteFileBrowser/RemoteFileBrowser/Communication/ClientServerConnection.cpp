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

	Http::RestCommunicator::Post(connectionSocket, std::move(hostname), "/api/RegisterConnection/", systemUniqueIdKey, systemUniqueIdValue);
	if (!Http::RestCommunicator::ReceiveResponse(connectionSocket))
	{
		Utilities::Logging::Log(L"[ERROR] Server didn't accept system unique ID.");
		return;
	}

	// Find my port out

	sockaddr_in6 socketAddress;
	int socketAddressLength = sizeof(socketAddress);

	auto result = getsockname(connectionSocket, reinterpret_cast<sockaddr*>(&socketAddress), &socketAddressLength);
	Assert(result == ERROR_SUCCESS);
	
	// Start listening for connection

	Tcp::Listener listener;
	listener.RunAsync(socketAddress.sin6_addr, socketAddress.sin6_port, [](SOCKET incomingSocket, sockaddr_in6 clientAddress)
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
		
		static const string ipKey("IpAddress");
		auto clientIp = clientInfo.find(ipKey);

		if (clientIp == clientInfo.end())	// Server sent us garbage
		{									// Drop connection from our side
			return;
		}

		// Try parse as IPv6, 
		// f it fails, try IPv4
		IN_ADDR inAddr;
		IN6_ADDR in6Addr;

		if (InetPtonA(AF_INET6, clientIp->second.c_str(), &in6Addr) != 1)
		{
			if (InetPtonA(AF_INET, clientIp->second.c_str(), &inAddr) != 1)
			{
				return;
			}
			else
			{
				IN6_SET_ADDR_V4MAPPED(&in6Addr, &inAddr);
			}
		}

		if (in6Addr.u.Word[0] == 0 && in6Addr.u.Word[1] == 0 && in6Addr.u.Word[2] == 0 && in6Addr.u.Word[3] == 0 &&
			in6Addr.u.Word[4] == 0 && in6Addr.u.Word[5] == 0 && in6Addr.u.Word[6] == 0 && in6Addr.u.Word[7] == 0)	// IP can't be 0
		{
			return;
		}

		listener.WhitelistIP(in6Addr);
		Http::RestCommunicator::SendResponse(connectionSocket, true);
	}	
}
