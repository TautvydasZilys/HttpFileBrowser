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

	void DecodeUrlInline(std::string& url);

	inline std::string DecodeUrl(const std::string& url)
	{
		std::string result = url;
		DecodeUrlInline(result);
		return result;
	}

	void EncodeUrlInline(std::string& url);

	inline std::string EncodeUrl(const std::string& url)
	{
		std::string result = url;
		EncodeUrlInline(result);
		return result;
	}

	void RemoveLastPathComponentInline(std::string& path);

	inline std::string RemoveLastPathComponent(const std::string& path)
	{
		std::string result = path;
		RemoveLastPathComponentInline(result);
		return result;
	}

	std::string CombinePaths(const std::string& left, const std::string& right);

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

	std::vector<uint8_t> ReadFileToVector(const std::wstring& path);
};

#if _DEBUG

#define Assert(x) do { if (!(x) && IsDebuggerPresent()) { __debugbreak(); } } while (0)

#else

#define Assert(x)

#endif