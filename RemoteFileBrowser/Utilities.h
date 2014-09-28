#pragma once

namespace Utilities
{
	namespace Logging
	{
		std::wstring Win32ErrorToMessage(int win32ErrorCode);

		void Log(const wchar_t* message);
		inline void Log(const std::wstring& message);

		void Error(int win32ErrorCode, const std::wstring& message);
		void FatalError(int win32ErrorCode, const std::wstring& message);

		template <typename Action>
		inline void PerformActionIfFailed(bool failed, const std::wstring& message, Action action);
		inline void LogErrorIfFailed(bool failed, const std::wstring& message);
		inline void LogFatalErrorIfFailed(bool failed, const std::wstring& message);
	}

	namespace Encoding
	{
		std::wstring Utf8ToUtf16(const char* str, size_t strLength);
		inline std::wstring Utf8ToUtf16(const std::string& str);

		std::string Utf16ToUtf8(const wchar_t* wstr, size_t wstrLength);
		inline std::string Utf16ToUtf8(const std::wstring& wstr);

		void DecodeUrlInline(std::string& url);
		inline std::string DecodeUrl(const std::string& url);

		void EncodeUrlInline(std::string& url);
		inline std::string EncodeUrl(const std::string& url);
	}

	namespace FileSystem
	{
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

		void RemoveLastPathComponentInline(std::string& path);
		inline std::string RemoveLastPathComponent(const std::string& path);

		std::string CombinePaths(const std::string& left, const std::string& right);

		std::string FormatFileSizeString(uint64_t size);

		FileStatus QueryFileStatus(const std::wstring& path);
		std::vector<FileInfo> EnumerateFiles(std::wstring path);
		std::vector<std::string> EnumerateSystemVolumes();

		std::vector<uint8_t> ReadFileToVector(const std::wstring& path);
	}
};

#include "Utilities.inl"