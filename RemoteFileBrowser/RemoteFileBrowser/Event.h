#pragma once

class Event
{
private:
	HANDLE m_EventHandle;

public:
	inline Event(bool manualReset) :
		m_EventHandle(CreateEvent(nullptr, manualReset, FALSE, nullptr))
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
		WaitForSingleObject(m_EventHandle, INFINITE);
	}
};

