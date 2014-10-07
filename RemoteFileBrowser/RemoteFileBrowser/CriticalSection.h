#pragma once

class CriticalSection
{
private:
	CRITICAL_SECTION m_CriticalSection;

public:
	inline CriticalSection()
	{
		InitializeCriticalSection(&m_CriticalSection);
	}

	inline ~CriticalSection()
	{
		DeleteCriticalSection(&m_CriticalSection);
	}

	inline void Enter()
	{
		EnterCriticalSection(&m_CriticalSection);
	}

	inline void Leave()
	{
		LeaveCriticalSection(&m_CriticalSection);
	}

	class Lock
	{
	private:
		CriticalSection& m_Section;

	public:
		inline Lock(CriticalSection criticalSection) :
			m_Section(criticalSection)
		{
			m_Section.Enter();
		}

		inline ~Lock()
		{
			m_Section.Leave();
		}
	};
};

