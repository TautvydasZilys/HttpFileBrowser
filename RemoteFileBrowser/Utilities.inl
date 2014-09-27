// Implementation file for Utilities.h inline functions


#if _DEBUG

#define Assert(x) do { if (!(x) && IsDebuggerPresent()) { __debugbreak(); } } while (0)

#else

#define Assert(x)

#endif

// Logging

template <typename Action>
inline static void Utilities::Logging::PerformActionIfFailed(bool failed, const std::wstring& message, Action action)
{
	if (!failed)
	{
		return;
	}

	Assert(false);
	auto errorCode = WSAGetLastError();
	action(errorCode, message);
}

inline static void Utilities::Logging::LogErrorIfFailed(bool failed, const std::wstring& message)
{
	PerformActionIfFailed(failed, message, [](int errorCode, const std::wstring& msg)
	{
		Error(errorCode, msg);
	});
}

inline static void Utilities::Logging::LogFatalErrorIfFailed(bool failed, const std::wstring& message)
{
	PerformActionIfFailed(failed, message, [](int errorCode, const std::wstring& msg)
	{
		FatalError(errorCode, msg);
	});
}

// Encoding

inline void Utilities::Logging::Log(const std::wstring& message)
{
	Log(message.c_str());
}

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
