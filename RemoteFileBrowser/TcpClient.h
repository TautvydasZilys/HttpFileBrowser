#pragma once

namespace TcpClient
{
	template <typename ConnectionHandler>
	inline void Connect(const std::wstring& hostName, ConnectionHandler connectionHandler);
};

#include "TcpClient.inl"