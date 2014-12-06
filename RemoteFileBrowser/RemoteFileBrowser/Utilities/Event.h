#pragma once

class Event
{
private:
	HANDLE m_EventHandle;

#if _DEBUG
	volatile bool m_IsSet;
	const bool m_IsManualReset;
#endif

public:
	inline Event(bool manualReset) :
		m_EventHandle(CreateEventEx(nullptr, nullptr, manualReset ? CREATE_EVENT_MANUAL_RESET : 0, EVENT_MODIFY_STATE | SYNCHRONIZE))
#if _DEBUG
		, m_IsSet(false), m_IsManualReset(manualReset)
#endif
	{
	}

	Event(const Event& other) = delete;
	Event& operator=(const Event& other) = delete;

	inline ~Event()
	{
		CloseHandle(m_EventHandle);
	}

	inline void Set()
	{
		SetEvent(m_EventHandle);

#if _DEBUG
		m_IsSet = true;
		MemoryBarrier();
#endif
	}

	inline void Reset()
	{
		ResetEvent(m_EventHandle);

#if _DEBUG
		m_IsSet = false;
		MemoryBarrier();
#endif
	}

	inline void Wait()
	{
		auto result = WaitForSingleObjectEx(m_EventHandle, INFINITE, FALSE);
		Assert(result == WAIT_OBJECT_0);

#if _DEBUG
		if (!m_IsManualReset) m_IsSet = false;
		MemoryBarrier();
#endif
	}
};

