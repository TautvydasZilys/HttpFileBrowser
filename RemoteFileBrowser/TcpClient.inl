// TcpClient inline implementation file

template <typename ConnectionHandler>
TcpClient::TcpClient(ConnectionHandler callback) :
	m_Callback(callback)
{
	using namespace Utilities;

	m_Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	Logging::LogFatalErrorIfFailed(m_Socket == INVALID_SOCKET, L"Failed to open a TCP socket: ");
}

template <typename ConnectionHandler>
TcpClient::~TcpClient()
{
}
