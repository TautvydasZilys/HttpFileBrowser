#pragma once

template <typename ConnectionHandler>
class TcpClient
{
	ConnectionHandler m_Callback;
	SOCKET m_Socket;

public:
	TcpClient(ConnectionHandler callback);
	~TcpClient();
};

#include "TcpClient.inl"