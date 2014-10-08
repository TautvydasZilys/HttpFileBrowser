#pragma once

namespace Tcp
{
	namespace Client
	{
		template <typename ConnectionHandler>
		inline void Connect(const std::wstring& hostName, int port, ConnectionHandler connectionHandler);
	}

	#include "Client.inl"
}