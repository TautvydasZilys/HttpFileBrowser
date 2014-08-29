#pragma once

namespace TcpListener
{
	void Initialize();
	void Cleanup();

	void Run(int port, std::function<void(SOCKET, sockaddr_in)> callback);
}