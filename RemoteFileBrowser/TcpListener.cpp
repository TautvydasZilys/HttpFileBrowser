#include "PrecompiledHeader.h"
#include "TcpListener.h"
#include "Utilities.h"

namespace TcpListener
{
	template <typename Action>
	inline static void PerformActionIfFailed(bool failed, const std::wstring& message, Action action)
	{
		if (!failed)
		{
			return;
		}

		Assert(false);
		auto errorCode = WSAGetLastError();
		action(errorCode, message);
	}

	inline static void LogErrorIfFailed(bool failed, const std::wstring& message)
	{
		PerformActionIfFailed(failed, message, [](int errorCode, const std::wstring& msg)
		{
			Utilities::Error(errorCode, msg);
		});
	}

	inline static void LogFatalErrorIfFailed(bool failed, const std::wstring& message)
	{
		PerformActionIfFailed(failed, message, [](int errorCode, const std::wstring& msg)
		{
			Utilities::FatalError(errorCode, msg);
		});
	}

	void Initialize()
	{
		WSAData wsaData;
		int wsaError = ERROR_SUCCESS;

		ZeroMemory(&wsaData, sizeof(wsaData));

		Utilities::Log(L"Initializing WinSock 2.2.");

		do
		{
			auto wsaResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
			Assert(wsaResult == NO_ERROR);

			if (wsaResult != 0)
			{
				wsaError = WSAGetLastError();
				Sleep(100);
			}
		}
		while (wsaError == WSASYSNOTREADY ||
				wsaError == WSAEINPROGRESS ||
				wsaError == WSAEPROCLIM);

		LogFatalErrorIfFailed(wsaError != ERROR_SUCCESS, L"Failed to initialize WinSock: ");
	}

	void Cleanup()
	{
		Utilities::Log(L"Cleaning up WinSock.");
		
		auto cleanupResult = WSACleanup();
		LogErrorIfFailed(cleanupResult != NO_ERROR, L"Failed to cleanup WinSock: ");
	}

	static SOCKET CreateListeningSocket(int port)
	{
		// Open listening socket

		auto listeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		LogFatalErrorIfFailed(listeningSocket == INVALID_SOCKET, L"Failed to open a listening socket: ");

		// Make it able reuse the address

		BOOL trueValue = TRUE;
		auto result = setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&trueValue), sizeof(trueValue));
		LogFatalErrorIfFailed(result == SOCKET_ERROR, L"Failed to set the listening socket to reuse its address: ");

		// Bind it to port

		sockaddr_in inAddress;
		ZeroMemory(&inAddress, sizeof(sockaddr_in));

		inAddress.sin_family = AF_INET;
		inAddress.sin_addr.s_addr = INADDR_ANY;
		inAddress.sin_port = htons(port);

		result = bind(listeningSocket, reinterpret_cast<sockaddr*>(&inAddress), sizeof(inAddress));
		LogFatalErrorIfFailed(result == SOCKET_ERROR, L"Failed to bind the listening socket: ");

		// Listen on the socket

		result = listen(listeningSocket, SOMAXCONN);
		LogFatalErrorIfFailed(result == SOCKET_ERROR, L"Failed to listen on the listening socket: ");

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

		Utilities::Log(msgBuffer);

		std::thread t([callback, acceptedSocket, clientAddress](){ callback(acceptedSocket, clientAddress); });
		t.detach();
	}

	void Run(int port, std::function<void(SOCKET, sockaddr_in)> callback)
	{
		auto listeningSocket = CreateListeningSocket(port);

		while (true)
		{
			sockaddr_in clientAddress;
			int clientAddressSize = sizeof(clientAddress);
			auto acceptedSocket = accept(listeningSocket, reinterpret_cast<sockaddr*>(&clientAddress), &clientAddressSize);
			
			if (acceptedSocket != INVALID_SOCKET)
			{
				StartIncomingConnectionThread(callback, acceptedSocket, clientAddress);
			}
			else
			{
				LogErrorIfFailed(true, L"Failed to accept connection: ");
			}
		}

		Utilities::Log(L"Closing listening socket.");
		auto closeResult = closesocket(listeningSocket);
		LogErrorIfFailed(closeResult == SOCKET_ERROR, L"Failed to close listening socket: ");
	}
}