inline SOCKET TcpListener::CreateListeningSocket(int port)
{
	using namespace Utilities;

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

template <typename Callback>
inline void TcpListener::StartIncomingConnectionThread(Callback callback, SOCKET acceptedSocket, const sockaddr_in& clientAddress)
{
	const int bufferSize = 256;
	wchar_t msgBuffer[bufferSize];
	swprintf_s(msgBuffer, bufferSize, L"Accepted connection from %d.%d.%d.%d.",
		clientAddress.sin_addr.S_un.S_un_b.s_b1,
		clientAddress.sin_addr.S_un.S_un_b.s_b2,
		clientAddress.sin_addr.S_un.S_un_b.s_b3,
		clientAddress.sin_addr.S_un.S_un_b.s_b4);

	Logging::Log(msgBuffer);

	std::thread t(callback, acceptedSocket, clientAddress);
	t.detach();
}

inline bool TcpListener::IsIpWhitelisted(ULONG ip)
{
	CriticalSection::Lock lock(m_IpWhitelistCriticalSection);

	for (const auto whitelistedIp : m_IpWhitelist)
	{
		if (whitelistedIp == ip)
		{
			return true;
		}
	}

	return false;
}

template <typename Callback>
void TcpListener::Run(int port, Callback callback)
{
	auto listeningSocket = CreateListeningSocket(port);

	while (m_Running)
	{
		sockaddr_in clientAddress;
		int clientAddressSize = sizeof(clientAddress);
		auto acceptedSocket = accept(listeningSocket, reinterpret_cast<sockaddr*>(&clientAddress), &clientAddressSize);
			
		if (acceptedSocket != INVALID_SOCKET)
		{
			if (IsIpWhitelisted(clientAddress.sin_addr.S_un.S_addr))
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

template <typename Callback>
void TcpListener::RunAsync(int port, Callback callback)
{
	Assert(!m_Running);

	m_Running = true;
	m_ListeningThread = std::thread([this, port, callback]()
	{
		Run(port, callback);
	});
}