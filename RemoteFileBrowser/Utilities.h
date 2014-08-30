#pragma once

namespace Utilities
{
	std::wstring Win32ErrorToMessage(int win32ErrorCode);

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

	std::string DecodeUrl(const std::string& url);
	std::string EncodeUrl(const std::string& url);

	std::string FormatFileSize(uint64_t size);

	enum class FileStatus
	{
		FileNotFound,
		AccessDenied,
		Directory,
		File
	};

	struct FileInfo
	{
		std::string fileName;
		FileStatus fileStatus;
		std::string dateModified;
		uint64_t fileSize;

		FileInfo(const std::string& fileName, FileStatus fileStatus, const std::string& dateModified, uint64_t fileSize);
		FileInfo(std::string&& fileName, FileStatus fileStatus, std::string&& dateModified, uint64_t fileSize);
	};

	FileStatus QueryFileStatus(const std::wstring& path);
	std::vector<FileInfo> EnumerateFiles(std::wstring path);
	std::vector<std::string> EnumerateSystemVolumes();

	// Very specialized function: appends file length, "\r\n\r\n" and file contents to the target buffer
	bool AppendFileLengthAndReadItWholeTo(const std::wstring& path, std::string& targetBuffer);
};

#if _DEBUG

#define Assert(x) do { if (!(x) && IsDebuggerPresent()) { __debugbreak(); } } while (0)

#else

#define Assert(x)

#endif