#include "PrecompiledHeader.h"
#include "Listener.h"

using namespace Tcp;

Listener::Listener(bool acceptAnonymousConnections) :
	m_Running(false), m_AcceptAnonymousConnections(acceptAnonymousConnections)
{
}

Listener::~Listener()
{
	if (m_Running)
	{
		Stop();
	}
}

void Listener::Stop()
{
	Assert(m_Running);
	m_Running = false;
	m_ListeningThread.join();
}

void Listener::WhitelistIP(const IN6_ADDR& ip)
{
	CriticalSection::Lock lock(m_IpWhitelistCriticalSection);
	m_IpWhitelist.push_back(ip);
}