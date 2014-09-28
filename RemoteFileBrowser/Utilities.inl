// Implementation file for Utilities.h inline functions


#if _DEBUG

#define Assert(x) do { if (!(x) && IsDebuggerPresent()) { __debugbreak(); } } while (0)

#else

#define Assert(x)

#endif

// Logging

template <typename Action>
inline void Utilities::Logging::PerformActionIfFailed(bool failed, const std::wstring& message, Action action)
{
	if (!failed)
	{
		return;
	}

	Assert(false);
	auto errorCode = WSAGetLastError();
	action(errorCode, message);
}

inline void Utilities::Logging::LogErrorIfFailed(bool failed, const std::wstring& message)
{
	PerformActionIfFailed(failed, message, [](int errorCode, const std::wstring& msg)
	{
		Error(errorCode, msg);
	});
}

inline void Utilities::Logging::LogFatalErrorIfFailed(bool failed, const std::wstring& message)
{
	PerformActionIfFailed(failed, message, [](int errorCode, const std::wstring& msg)
	{
		FatalError(errorCode, msg);
	});
}

template <typename Message>
inline void Utilities::Logging::OutputMessages(const Message& message)
{
	OutputMessage(message);
}

template <typename FirstMessage, typename ...Message>
inline void Utilities::Logging::OutputMessages(const FirstMessage& message, Message&&... messages)
{
	OutputMessage(message);
	OutputMessages(std::forward<Message>(messages)...);
}

inline void Utilities::Logging::OutputMessage(const std::wstring& message)
{
	OutputMessage(message.c_str());
}

template <typename ...Message>
inline void Utilities::Logging::Log(Message&& ...message)
{
	using namespace std;

	static mutex s_LogMutex;
	lock_guard<mutex> lock(s_LogMutex);

	OutputCurrentTimestamp();
	OutputMessages(std::forward<Message>(message)...);
	OutputMessage(L"\r\n");
}


// Encoding

inline std::wstring Utilities::Encoding::Utf8ToUtf16(const std::string& str)
{
	return Utf8ToUtf16(str.c_str(), str.length());
}

inline std::string Utilities::Encoding::Utf16ToUtf8(const std::wstring& wstr)
{
	return Utf16ToUtf8(wstr.c_str(), wstr.length());
}

inline std::string Utilities::Encoding::DecodeUrl(const std::string& url)
{
	std::string result = url;
	DecodeUrlInline(result);
	return result;
}

inline std::string Utilities::Encoding::EncodeUrl(const std::string& url)
{
	std::string result = url;
	EncodeUrlInline(result);
	return result;
}

// File system

inline std::string Utilities::FileSystem::RemoveLastPathComponent(const std::string& path)
{
	std::string result = path;
	RemoveLastPathComponentInline(result);
	return result;
}
