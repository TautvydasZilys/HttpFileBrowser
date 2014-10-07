#include "PrecompiledHeader.h"
#include "TcpListener.h"


TcpListener::TcpListener() :
	m_Running(false)
{
}

TcpListener::~TcpListener()
{
	if (m_Running)
	{
		Stop();
	}
}

void TcpListener::Stop()
{
	Assert(m_Running);
	m_Running = false;
	m_ListeningThread.join();
}

void TcpListener::WhitelistIP(ULONG ip)
{
	CriticalSection::Lock lock(m_IpWhitelistCriticalSection);
	m_IpWhitelist.push_back(ip);
}