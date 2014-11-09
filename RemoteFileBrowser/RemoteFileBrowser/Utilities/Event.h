#pragma once

class Event
{
private:
	HANDLE m_EventHandle;

public:
	inline Event(bool manualReset) :
		m_EventHandle(CreateEventEx(nullptr, nullptr, manualReset ? CREATE_EVENT_MANUAL_RESET : 0, EVENT_MODIFY_STATE))
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
	}

	inline void Reset()
	{
		ResetEvent(m_EventHandle);
	}

	inline void Wait()
	{
		WaitForSingleObjectEx(m_EventHandle, INFINITE, FALSE);
	}
};

