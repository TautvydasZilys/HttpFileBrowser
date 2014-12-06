// Implementation file for Utilities.h inline functions


#if _DEBUG

#define Assert(x) do { if (!(x) && IsDebuggerPresent()) { __debugbreak(); } } while (0)

#else

#define Assert(x)

#endif

// Logging

inline void Utilities::Logging::Win32ErrorToMessageInline(int win32ErrorCode, wchar_t (&buffer)[kBufferSize])
{
	FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, win32ErrorCode, 0, buffer, kBufferSize, nullptr);
}

inline void Utilities::Logging::Win32ErrorToMessageInline(int win32ErrorCode, char (&buffer)[kBufferSize])
{
	wchar_t wBuffer[kBufferSize];

	auto messageLength = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, win32ErrorCode, 0, wBuffer, kBufferSize, nullptr);
	Utilities::Encoding::Utf16ToUtf8Inline(wBuffer, messageLength + 1, buffer, kBufferSize);
}

template <typename Message>
inline void Utilities::Logging::OutputMessages(const Message& message)
{
	OutputMessage(message);
}

template <typename FirstMessage, typename ...Message>
inline void Utilities::Logging::OutputMessages(const FirstMessage& message, Message&& ...messages)
{
	OutputMessage(message);
	OutputMessages(std::forward<Message>(messages)...);
}

inline void Utilities::Logging::OutputMessage(const std::string& message)
{
	OutputMessage(message.c_str(), message.length());
}

inline void Utilities::Logging::OutputMessage(const char* message)
{
	OutputMessage(message, strlen(message));
}

template <int Length>
static inline void OutputMessage(const char(&message)[Length])
{
	OutputMessage(message, Length);
}

template <typename ...Message>
inline void Utilities::Logging::Log(Message&& ...message)
{
	CriticalSection::Lock lock(s_LogCriticalSection);

	OutputCurrentTimestamp();
	OutputMessages(std::forward<Message>(message)...);
	OutputMessage("\r\n");
}

template <typename ...Message>
inline void Utilities::Logging::Error(int win32ErrorCode, Message&& ...message)
{
	char errorMessage[kBufferSize];

	Win32ErrorToMessageInline(win32ErrorCode, errorMessage);
	Log(std::forward<Message>(message)..., errorMessage);
}

template <typename ...Message>
inline void Utilities::Logging::FatalError(int win32ErrorCode, Message&& ...message)
{
	char errorMessage[kBufferSize];

	Win32ErrorToMessageInline(win32ErrorCode, errorMessage);
	Log("Terminating due to critical error:\r\n\t\t", std::forward<Message>(message)..., errorMessage);

	Terminate(win32ErrorCode);
}

template <typename Action, typename ...Message>
inline void Utilities::Logging::PerformActionIfFailed(bool failed, Action action, Message&& ...message)
{
	if (!failed)
	{
		return;
	}

	Assert(false);
	auto errorCode = GetLastError();
	action(errorCode, std::forward<Message>(message)...);
}

template <typename ...Message>
inline void Utilities::Logging::LogErrorIfFailed(bool failed, Message&& ...message)
{
	PerformActionIfFailed(failed, [](int errorCode, Message&& ...msg)
	{
		Error(errorCode, std::forward<Message>(msg)...);
	}, std::forward<Message>(message)...);
}

template <typename ...Message>
inline void Utilities::Logging::LogFatalErrorIfFailed(bool failed, Message&& ...message)
{
	PerformActionIfFailed(failed, [](int errorCode, Message&& ...msg)
	{
		FatalError(errorCode, std::forward<Message>(msg)...);
	}, std::forward<Message>(message)...);
}

// Algorithms

template <typename T, typename Predicate>
void Utilities::Algorithms::FilterVector(std::vector<T>& items, Predicate&& predicate)
{
	for (size_t i = 0; i < items.size(); i++)
	{
		if (!predicate(items[i]))
		{
			if (i < items.size() - 1)
			{
				items[i] = std::move(items[items.size() - 1]);
				i--;
			}

			items.pop_back();
		}
	}
}

// Encoding

inline std::wstring Utilities::Encoding::Utf8ToUtf16(const std::string& str)
{
	return Utf8ToUtf16(str.c_str(), str.length());
}

inline size_t Utilities::Encoding::Utf8ToUtf16Inline(const std::string& str, wchar_t* destination, size_t destinationLength)
{
	return Utf8ToUtf16Inline(str.c_str(), str.length(), destination, destinationLength);
}

inline std::string Utilities::Encoding::Utf16ToUtf8(const std::wstring& wstr)
{
	return Utf16ToUtf8(wstr.c_str(), wstr.length());
}

inline size_t Utilities::Encoding::Utf16ToUtf8Inline(const std::wstring& wstr, char* destination, size_t destinationLength)
{
	return Utf16ToUtf8Inline(wstr.c_str(), wstr.length(), destination, destinationLength);
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

inline std::string Utilities::Encoding::EncodeBase64(const std::string& data)
{
	std::string result = data;
	EncodeBase64Inline(result);
	return result;
}

template <size_t bufferLength>
void Utilities::Encoding::IpToString(int ipFamily, void* ipAddress, char (&buffer)[bufferLength])
{
#if !PHONE
	auto strPtr = inet_ntop(ipFamily, ipAddress, buffer, bufferLength);
	Assert(strPtr == buffer);
#else
	Assert(bufferLength > strlen("ImplementMePls"));
	strcpy_s(buffer, "ImplementMePls");
#endif
}

// File system

inline std::string Utilities::FileSystem::RemoveLastPathComponent(const std::string& path)
{
	std::string result = path;
	RemoveLastPathComponentInline(result);
	return result;
}

template <typename WideStr>
inline std::vector<Utilities::FileSystem::FileInfo> Utilities::FileSystem::EnumerateAndSortFiles(WideStr&& path)
{
	auto files = EnumerateFiles(std::forward<WideStr>(path));
	SortFiles(files);
	return files;
}

inline HANDLE Utilities::FileSystem::CreateFilePortable(const std::wstring& path, DWORD desiredAccess, DWORD shareMode, DWORD creationDisposition)
{
#if !PHONE
	return CreateFileW(path.c_str(), desiredAccess, shareMode, nullptr, creationDisposition, FILE_ATTRIBUTE_NORMAL, nullptr);
#else
	return CreateFile2(path.c_str(), desiredAccess, shareMode, creationDisposition, nullptr);
#endif
}

// String

inline bool Utilities::String::CaseInsensitiveComparer::operator()(const std::string& left, const std::string& right)
{
	return _stricmp(left.c_str(), right.c_str()) == 0;
}

inline size_t Utilities::String::CaseInsensitiveHasher::operator()(const std::string& str)
{
	auto length = str.length();
	std::string lowerCaseStr;
	lowerCaseStr.resize(length);

	for (size_t i = 0; i < length; i++)
	{
		lowerCaseStr[i] = tolower(str[i]);
	}

	return std::hash<std::string>()(lowerCaseStr);
}
