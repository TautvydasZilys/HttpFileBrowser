#pragma once

namespace TcpClient
{
	template <typename ConnectionHandler>
	inline void Connect(const std::wstring& hostName, int port, ConnectionHandler connectionHandler);
};

#include "TcpClient.inl"