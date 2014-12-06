inline SOCKET Listener::CreateListeningSocket(const in6_addr& address, uint16_t port)
{
	using namespace Utilities;

	// Open listening socket

	auto listeningSocket = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
	Logging::LogFatalErrorIfFailed(listeningSocket == INVALID_SOCKET, "Failed to open a TCP socket: ");

	// Make it able reuse the address, make it dual mode and non blocking

	BOOL trueValue = TRUE;
	auto result = setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&trueValue), sizeof(trueValue));
	Logging::LogFatalErrorIfFailed(result == SOCKET_ERROR, "Failed to set the listening socket to reuse its address: ");

	BOOL falseValue = FALSE;
	result = setsockopt(listeningSocket, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<const char*>(&falseValue), sizeof(falseValue));
	Logging::LogFatalErrorIfFailed(result == SOCKET_ERROR, "Failed to set the listening socket to accept IPv4 connections: ");

	u_long nonBlocking = TRUE;
	result = ioctlsocket(listeningSocket, FIONBIO, &nonBlocking);
	Logging::LogFatalErrorIfFailed(result == SOCKET_ERROR, "Failed to set the listening socket to async mode: ");

	// Bind it to port

	sockaddr_in6 inAddress;
	ZeroMemory(&inAddress, sizeof(inAddress));

	inAddress.sin6_family = AF_INET6;
	inAddress.sin6_addr = address;
	inAddress.sin6_port = port;
	
	result = ::bind(listeningSocket, reinterpret_cast<sockaddr*>(&inAddress), sizeof(inAddress));
	Logging::LogFatalErrorIfFailed(result == SOCKET_ERROR, "Failed to bind the listening socket: ");

	// Listen on the socket

	result = listen(listeningSocket, SOMAXCONN);
	Logging::LogFatalErrorIfFailed(result == SOCKET_ERROR, "Failed to listen on the listening socket: ");

	return listeningSocket;
}

template <typename Callback>
inline void Listener::StartIncomingConnectionThread(Callback callback, SOCKET acceptedSocket, sockaddr_in6& clientAddress)
{
	const int bufferSize = 64;
	char msgBuffer[bufferSize];

	Utilities::Encoding::IpToString(AF_INET6, &clientAddress.sin6_addr, msgBuffer);
	Utilities::Logging::Log("Accepted connection from ", msgBuffer, ".");

	u_long nonBlocking = FALSE;
	auto result = ioctlsocket(acceptedSocket, FIONBIO, &nonBlocking);
	Utilities::Logging::LogFatalErrorIfFailed(result == SOCKET_ERROR, "Failed to set the listening socket to blocking mode: ");

	std::thread t(callback, acceptedSocket, clientAddress);
	t.detach();
}

inline bool Listener::IsIpWhitelisted(const IN6_ADDR& ip)
{
	CriticalSection::Lock lock(m_IpWhitelistCriticalSection);

	for (const auto whitelistedIp : m_IpWhitelist)
	{
		if (memcmp(&whitelistedIp, &ip, sizeof(IN6_ADDR)) == 0)
		{
			return true;
		}
	}

	return false;
}

template <typename Callback>
void Listener::Run(const in6_addr& address, uint16_t port, Callback callback)
{
	using namespace Utilities;
	auto listeningSocket = CreateListeningSocket(address, port);

	while (m_Running)
	{
		sockaddr_in6 clientAddress;
		int clientAddressSize = sizeof(clientAddress);
		auto acceptedSocket = accept(listeningSocket, reinterpret_cast<sockaddr*>(&clientAddress), &clientAddressSize);
			
		if (acceptedSocket != INVALID_SOCKET)
		{
			if (m_AcceptAnonymousConnections || IsIpWhitelisted(clientAddress.sin6_addr))
			{
				StartIncomingConnectionThread(callback, acceptedSocket, clientAddress);
			}
			else
			{
				closesocket(acceptedSocket);
			}
		}
		else if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			Logging::LogErrorIfFailed(true, "Failed to accept connection: ");
		}
		else
		{
			Utilities::System::Sleep(16);
		}
	}

	Logging::Log("Closing listening socket.");
	auto closeResult = closesocket(listeningSocket);
	Logging::LogErrorIfFailed(closeResult == SOCKET_ERROR, "Failed to close listening socket: ");
}

template <typename Callback>
void Listener::RunAsync(const in6_addr& address, uint16_t port, Callback callback)
{
	Assert(!m_Running);

	m_Running = true;
	m_ListeningThread = std::thread([this, address, port, callback]()
	{
		Run(address, port, callback);
	});
}