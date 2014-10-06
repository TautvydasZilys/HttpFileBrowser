// TcpClient inline implementation file

inline void LogEndpointAddress(const ADDRINFOW* addressInfo)
{
	Assert(addressInfo->ai_addr->sa_family == AF_INET);

	const int bufferSize = 256;
	wchar_t msgBuffer[bufferSize];

	swprintf_s(msgBuffer, bufferSize, L"Endpoint address: %d.%d.%d.%d:%d.",
		static_cast<uint8_t>(addressInfo->ai_addr->sa_data[2]),
		static_cast<uint8_t>(addressInfo->ai_addr->sa_data[3]),
		static_cast<uint8_t>(addressInfo->ai_addr->sa_data[4]),
		static_cast<uint8_t>(addressInfo->ai_addr->sa_data[5]),
		static_cast<uint16_t>(addressInfo->ai_addr->sa_data[0]) * 256 + addressInfo->ai_addr->sa_data[1]);

	Utilities::Logging::Log(msgBuffer);
}

template <typename ConnectionHandler>
inline void TcpClient::Connect(const std::wstring& hostName, int port, ConnectionHandler connectionHandler)
{
	using namespace Utilities;

	Logging::Log(L"Attempting to connect to \"", hostName, L"\".");

	// Setup socket

	auto connectionSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	Logging::LogFatalErrorIfFailed(connectionSocket == INVALID_SOCKET, L"Failed to open a TCP socket: ");

	// Resolve endpoint IP

	ADDRINFOW addressInfoHint;
	ADDRINFOW* addressInfo = nullptr;

	ZeroMemory(&addressInfoHint, sizeof(addressInfoHint));

	addressInfoHint.ai_family = AF_INET;
	addressInfoHint.ai_socktype = SOCK_STREAM;
	addressInfoHint.ai_protocol = IPPROTO_TCP;

	auto result = GetAddrInfoW(hostName.c_str(), nullptr, &addressInfoHint, &addressInfo);
	Logging::LogErrorIfFailed(result != ERROR_SUCCESS, L"Failed to get address info: ");
	if (result != ERROR_SUCCESS) goto cleanup;

	*reinterpret_cast<uint16_t*>(addressInfo->ai_addr->sa_data) = htons(port);
	LogEndpointAddress(addressInfo);

	result = connect(connectionSocket, addressInfo->ai_addr, addressInfo->ai_addrlen);
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
