#pragma once

namespace TcpListener
{
	void Run(int port, std::function<void(SOCKET, sockaddr_in)> callback);
}