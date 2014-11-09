#pragma once

class CriticalSection
{
private:
	CRITICAL_SECTION m_CriticalSection;

public:
	inline CriticalSection()
	{
#if _DEBUG
		DWORD flags = 0;
#else
		DWORD flags = CRITICAL_SECTION_NO_DEBUG_INFO;
#endif

		InitializeCriticalSectionEx(&m_CriticalSection, 2000, flags);
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
		inline Lock(CriticalSection& criticalSection) :
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

