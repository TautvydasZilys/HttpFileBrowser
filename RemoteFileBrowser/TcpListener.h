#pragma once

namespace TcpListener
{
	void Run(int port, int acceptConnectionOnlyFromIp, std::function<void(SOCKET, sockaddr_in)> callback);
}