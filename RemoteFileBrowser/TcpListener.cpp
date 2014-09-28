#include "PrecompiledHeader.h"
#include "TcpListener.h"

namespace TcpListener
{
	using namespace Utilities;

	static SOCKET CreateListeningSocket(int port)
	{
		// Open listening socket

		auto listeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		Logging::LogFatalErrorIfFailed(listeningSocket == INVALID_SOCKET, L"Failed to open a TCP socket: ");

		// Make it able reuse the address

		BOOL trueValue = TRUE;
		auto result = setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&trueValue), sizeof(trueValue));
		Logging::LogFatalErrorIfFailed(result == SOCKET_ERROR, L"Failed to set the listening socket to reuse its address: ");

		// Bind it to port

		sockaddr_in inAddress;
		ZeroMemory(&inAddress, sizeof(sockaddr_in));

		inAddress.sin_family = AF_INET;
		inAddress.sin_addr.s_addr = INADDR_ANY;
		inAddress.sin_port = htons(port);

		result = bind(listeningSocket, reinterpret_cast<sockaddr*>(&inAddress), sizeof(inAddress));
		Logging::LogFatalErrorIfFailed(result == SOCKET_ERROR, L"Failed to bind the listening socket: ");

		// Listen on the socket

		result = listen(listeningSocket, SOMAXCONN);
		Logging::LogFatalErrorIfFailed(result == SOCKET_ERROR, L"Failed to listen on the listening socket: ");

		return listeningSocket;
	}

	static void StartIncomingConnectionThread(std::function<void(SOCKET, sockaddr_in)> callback, SOCKET acceptedSocket, const sockaddr_in& clientAddress)
	{
		const int bufferSize = 256;
		wchar_t msgBuffer[bufferSize];
		swprintf_s(msgBuffer, bufferSize, L"Accepted connection from %d.%d.%d.%d.",
			clientAddress.sin_addr.S_un.S_un_b.s_b1,
			clientAddress.sin_addr.S_un.S_un_b.s_b2,
			clientAddress.sin_addr.S_un.S_un_b.s_b3,
			clientAddress.sin_addr.S_un.S_un_b.s_b4);

		Logging::Log(msgBuffer);

		std::thread t([callback, acceptedSocket, clientAddress](){ callback(acceptedSocket, clientAddress); });
		t.detach();
	}

	void Run(int port, int acceptConnectionOnlyFromIp, std::function<void(SOCKET, sockaddr_in)> callback)
	{
		auto listeningSocket = CreateListeningSocket(port);

		while (true)
		{
			sockaddr_in clientAddress;
			int clientAddressSize = sizeof(clientAddress);
			auto acceptedSocket = accept(listeningSocket, reinterpret_cast<sockaddr*>(&clientAddress), &clientAddressSize);
			
			if (acceptedSocket != INVALID_SOCKET)
			{
				if (acceptConnectionOnlyFromIp == 0 || acceptConnectionOnlyFromIp == clientAddress.sin_addr.S_un.S_addr)
				{
					StartIncomingConnectionThread(callback, acceptedSocket, clientAddress);
				}
				else
				{
					closesocket(acceptedSocket);
				}
			}
			else
			{
				Logging::LogErrorIfFailed(true, L"Failed to accept connection: ");
			}
		}

		Logging::Log(L"Closing listening socket.");
		auto closeResult = closesocket(listeningSocket);
		Logging::LogErrorIfFailed(closeResult == SOCKET_ERROR, L"Failed to close listening socket: ");
	}
}