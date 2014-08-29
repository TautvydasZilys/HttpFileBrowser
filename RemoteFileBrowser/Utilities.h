#pragma once

namespace Utilities
{
	void Log(const wchar_t* message);

	inline void Log(const std::wstring& message)
	{
		Log(message.c_str());
	}

	void Error(int win32ErrorCode, const std::wstring& message);
	void FatalError(int win32ErrorCode, const std::wstring& message);

	std::wstring Utf8ToUtf16(const char* str, size_t strLength);

	inline std::wstring Utf8ToUtf16(const std::string& str)
	{
		return Utf8ToUtf16(str.c_str(), str.length());
	}

	std::string Utf16ToUtf8(const wchar_t* wstr, size_t wstrLength);

	inline std::string Utf16ToUtf8(const std::wstring& wstr)
	{
		return Utf16ToUtf8(wstr.c_str(), wstr.length());
	}

	std::vector<std::string> EnumerateSystemVolumes();
};

#if _DEBUG

#define Assert(x) do { if (!(x) && IsDebuggerPresent()) { __debugbreak(); } } while (0)

#else

#define Assert(x)

#endif