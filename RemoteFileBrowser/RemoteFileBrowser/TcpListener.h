#pragma once

#include "CriticalSection.h"

class TcpListener
{
private:
	std::vector<ULONG> m_IpWhitelist;
	CriticalSection m_IpWhitelistCriticalSection;
	std::thread m_ListeningThread;
	volatile bool m_Running;

	static inline SOCKET CreateListeningSocket(int address, uint16_t port);
	inline bool IsIpWhitelisted(ULONG ip);

	template <typename Callback>
	inline void Run(int address, uint16_t port, Callback callback);

	template <typename Callback>
	static inline void StartIncomingConnectionThread(Callback callback, SOCKET acceptedSocket, const sockaddr_in& clientAddress);

public:
	TcpListener();
	~TcpListener();

	TcpListener(const TcpListener&) = delete;
	TcpListener& operator=(const TcpListener&) = delete;

	template <typename Callback>
	inline void RunAsync(int address, uint16_t port, Callback callback);
	void Stop();

	void WhitelistIP(ULONG ip);
};

#include "TcpListener.inl"