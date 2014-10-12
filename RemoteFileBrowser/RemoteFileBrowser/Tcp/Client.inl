// TcpClient inline implementation file

inline void LogEndpointAddress(sockaddr_in6* addressInfo)
{
	const int bufferSize = 64;
	wchar_t msgBuffer[bufferSize];

	auto msgPtr = InetNtop(AF_INET6, &addressInfo->sin6_addr, msgBuffer, bufferSize);
	Assert(msgPtr != nullptr);

	Utilities::Logging::Log(L"Endpoint address: ", msgBuffer, L".");
}

inline void LogEndpointAddress(sockaddr_in* addressInfo)
{
	const int bufferSize = 64;
	wchar_t msgBuffer[bufferSize];

	auto msgPtr = InetNtop(AF_INET, &addressInfo->sin_addr, msgBuffer, bufferSize);
	Assert(msgPtr != nullptr);

	Utilities::Logging::Log(L"Endpoint address: ", msgBuffer, L".");
}

template <typename ConnectionHandler>
inline void Client::Connect(const std::wstring& hostName, int port, ConnectionHandler connectionHandler)
{
	using namespace Utilities;

	Logging::Log(L"Attempting to connect to \"", hostName, L"\".");

	// Setup socket

	auto connectionSocket = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
	Logging::LogFatalErrorIfFailed(connectionSocket == INVALID_SOCKET, L"Failed to open a TCP socket: ");

	BOOL falseValue = FALSE;
	auto result = setsockopt(connectionSocket, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<const char*>(&falseValue), sizeof(falseValue));
	Logging::LogFatalErrorIfFailed(result == SOCKET_ERROR, L"Failed to set the connection socket to be able to connection via IPv4: ");

	// Resolve endpoint IP

	ADDRINFOW addressInfoHint;
	ADDRINFOW* addressInfo = nullptr;

	ZeroMemory(&addressInfoHint, sizeof(addressInfoHint));

	addressInfoHint.ai_family = AF_UNSPEC;
	addressInfoHint.ai_socktype = SOCK_STREAM;

	result = GetAddrInfoW(hostName.c_str(), nullptr, &addressInfoHint, &addressInfo);
	Logging::LogErrorIfFailed(result != ERROR_SUCCESS, L"Failed to get address info: ");
	if (result != ERROR_SUCCESS) goto cleanup;
	
	sockaddr_in6 remoteAddress;

	if (addressInfo->ai_family == AF_INET)
	{
		auto socketAddress = reinterpret_cast<sockaddr_in*>(addressInfo->ai_addr);
		SCOPE_ID scopeId;
		scopeId.Value = 0;

		IN6ADDR_SETV4MAPPED(&remoteAddress, &socketAddress->sin_addr, scopeId, htons(port));
		LogEndpointAddress(socketAddress);
	}
	else
	{
		remoteAddress = *reinterpret_cast<sockaddr_in6*>(addressInfo->ai_addr);
		remoteAddress.sin6_port = htons(port);
		LogEndpointAddress(&remoteAddress);
	}
	
	result = connect(connectionSocket, reinterpret_cast<sockaddr*>(&remoteAddress), sizeof(remoteAddress));
	Logging::LogErrorIfFailed(result != ERROR_SUCCESS, L"Failed to connect to the end point: ");
	if (result != ERROR_SUCCESS) goto cleanup;

	connectionHandler(connectionSocket, Utilities::Encoding::Utf16ToUtf8(hostName));

cleanup:
	closesocket(connectionSocket);

	if (addressInfo != nullptr)
	{
		FreeAddrInfoW(addressInfo);
	}
}
