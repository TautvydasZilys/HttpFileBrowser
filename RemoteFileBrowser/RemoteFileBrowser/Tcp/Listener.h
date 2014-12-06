#pragma once

#include "Utilities\CriticalSection.h"

namespace Tcp
{
	class Listener
	{
	private:
		bool m_AcceptAnonymousConnections;
		std::vector<IN6_ADDR> m_IpWhitelist;
		CriticalSection m_IpWhitelistCriticalSection;
		std::thread m_ListeningThread;
		volatile bool m_Running;

		static inline SOCKET CreateListeningSocket(const in6_addr& address, uint16_t port);
		inline bool IsIpWhitelisted(const IN6_ADDR& ip);

		template <typename Callback>
		inline void Run(const in6_addr& address, uint16_t port, Callback callback);

		template <typename Callback>
		static inline void StartIncomingConnectionThread(Callback callback, SOCKET acceptedSocket, sockaddr_in6& clientAddress);

	public:
		Listener(bool acceptAnonymousConnections = false);
		~Listener();

		Listener(const Listener&) = delete;
		Listener& operator=(const Listener&) = delete;

		template <typename Callback>
		inline void RunAsync(const in6_addr& address, uint16_t port, Callback callback);
		void Stop();

		void WhitelistIP(const IN6_ADDR& ip);
	};

	#include "Listener.inl"
}