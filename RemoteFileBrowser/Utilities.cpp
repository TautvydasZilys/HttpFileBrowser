#include "PrecompiledHeader.h"
#include "Utilities.h"

namespace Utilities
{
	using namespace std;

	static wofstream s_OutputFile;
	static const wchar_t kLogFileName[] = L"LogFile.log";

	void OutputMessage(const wchar_t* message)
	{
		if (IsDebuggerPresent())
		{
			OutputDebugStringW(message);
		}

		if (!s_OutputFile.is_open())
		{
			s_OutputFile.open(kLogFileName);
		}

		s_OutputFile << message;
	}

	inline void OutputMessage(const wstring& message)
	{
		OutputMessage(message.c_str());
	}

	wstring SystemTimeToString(SYSTEMTIME* systemTime)
	{
		const int bufferSize = 256;
		wchar_t buffer[bufferSize];

		auto dateLength = GetDateFormatEx(LOCALE_NAME_SYSTEM_DEFAULT, DATE_SHORTDATE, systemTime, nullptr, buffer + 1, bufferSize - 1, nullptr);
		buffer[dateLength] = ' ';
		auto timeLength = GetTimeFormatEx(LOCALE_NAME_SYSTEM_DEFAULT, 0, systemTime, nullptr, buffer + dateLength + 1, bufferSize - dateLength - 1);

		buffer[0] = '[';
		buffer[dateLength + timeLength] = ']';
		buffer[dateLength + timeLength + 1] = ' ';
		buffer[dateLength + timeLength + 2] = '\0';

		return buffer;
	}

	wstring GetCurrentTimestamp()
	{
		return SystemTimeToString(nullptr);
	}

	void Log(const wchar_t* message)
	{
		OutputMessage(GetCurrentTimestamp());
		OutputMessage(message);
		OutputMessage(L"\r\n");
	}

	wstring Win32ErrorToMessage(int win32ErrorCode)
	{
		const int bufferSize = 256;
		wchar_t buffer[bufferSize];

		FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, win32ErrorCode, 0, buffer, bufferSize, nullptr);
		return buffer;
	}

	void Terminate()
	{
		if (s_OutputFile.is_open())
		{
			s_OutputFile.close();
		}

		TerminateProcess(GetCurrentProcess(), -1);
	}

	void Error(int win32ErrorCode, const wstring& message)
	{
		auto errorMessage = Win32ErrorToMessage(win32ErrorCode);
		Log(message + errorMessage);
	}

	void FatalError(int win32ErrorCode, const wstring& message)
	{
		auto errorMessage = Win32ErrorToMessage(win32ErrorCode);

		Log(L"Terminating due to critical error:");
		Log(message + errorMessage);

		Terminate();
	}

	wstring Utf8ToUtf16(const char* str, size_t strLength)
	{
		if (strLength == 0) return wstring();

		auto bufferSize = 4 * strLength;
		std::unique_ptr<wchar_t[]> buffer(new wchar_t[bufferSize]);

		auto length = MultiByteToWideChar(CP_UTF8, 0, str, strLength, buffer.get(), bufferSize);
		Assert(length > 0);

		return wstring(buffer.get(), length);
	}

	string Utf16ToUtf8(const wchar_t* wstr, size_t wstrLength)
	{
		if (wstrLength == 0) return string();

		auto bufferSize = 8 * wstrLength;
		std::unique_ptr<char[]> buffer(new char[bufferSize]);

		auto length = WideCharToMultiByte(CP_UTF8, 0, wstr, wstrLength, buffer.get(), bufferSize, nullptr, nullptr);
		Assert(length > 0);

		return string(buffer.get(), length);
	}

	static char HexCharToNumber(char c)
	{
		if (c > '9')
		{
			if (c > 'a')
			{
				return c - 'a' + 10;
			}
			else
			{
				return c - 'A' + 10;
			}
		}
		else
		{
			return c - '0';
		}
	}

	string DecodeUrl(const string& url)
	{
		string decoded;
		auto urlLength = url.length();
		unsigned int i = 0;

		decoded.reserve(urlLength);

		while (i < urlLength)
		{
			if (url[i] != '%')
			{
				decoded += url[i];
				i++;
				continue;
			}

			if (urlLength - i < 3)
			{
				break;
			}

			decoded += static_cast<char>(HexCharToNumber(url[i + 1]) * 16 + HexCharToNumber(url[i + 2]));
			i += 3;
		}

		return decoded;
	}

	FileInfo::FileInfo(const string& fileName, FileStatus fileStatus, const string& dateModified) :
		fileName(std::move(fileName)), fileStatus(fileStatus), dateModified(std::move(dateModified))
	{
	}

	FileInfo::FileInfo(string&& fileName, FileStatus fileStatus, string&& dateModified) :
		fileName(std::move(fileName)), fileStatus(fileStatus), dateModified(std::move(dateModified))
	{
	}

	FileStatus QueryFileStatus(const wstring& path)
	{
		auto fileAttributes = GetFileAttributesW(path.c_str());

		if (fileAttributes == INVALID_FILE_ATTRIBUTES)
		{
			auto lastError = GetLastError();
			SetLastError(ERROR_SUCCESS);

			return lastError == ERROR_FILE_NOT_FOUND ? FileStatus::FileNotFound : FileStatus::AccessDenied;
		}

		return (fileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 ? FileStatus::Directory : FileStatus::File;
	}

	vector<FileInfo> EnumerateFiles(wstring path)
	{
		vector<FileInfo> result;

		if (path[path.length() - 1] == L'\\')
		{
			path += L"*.*";
		}
		else
		{
			path += L"\\*.*";
		}

		WIN32_FIND_DATA findData;
		auto findHandle = FindFirstFileW(path.c_str(), &findData);

		if (findHandle == INVALID_HANDLE_VALUE)
		{
			return result;
		}

		do
		{
			auto fileNameLength = wcslen(findData.cFileName);
			string fileName(Utf16ToUtf8(findData.cFileName, fileNameLength));

			SYSTEMTIME modifiedTime;
			FileTimeToSystemTime(&findData.ftLastWriteTime, &modifiedTime);
			string dateModified(Utf16ToUtf8(SystemTimeToString(&modifiedTime)));

			auto fileStatus = ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) ? FileStatus::Directory : FileStatus::File;

			result.emplace_back(std::move(fileName), fileStatus, std::move(dateModified));
		}
		while (FindNextFileW(findHandle, &findData) != FALSE);

		FindClose(findHandle);
		SetLastError(ERROR_SUCCESS);

		// Sort by name, but place directories first
		sort(begin(result), end(result), [](FileInfo& left, FileInfo& right) -> bool
		{
			if (left.fileStatus == right.fileStatus)
			{
				return left.fileName < right.fileName;
			}

			return left.fileStatus == FileStatus::Directory;
		});

		return result;
	}

	vector<string> EnumerateSystemVolumes()
	{
		const int bufferSize = 256;
		wchar_t buffer[bufferSize];
		vector<string> volumes;

		auto findHandle = FindFirstVolumeW(buffer, bufferSize);
		Assert(findHandle != INVALID_HANDLE_VALUE);
				
		do
		{
			wchar_t pathBuffer[bufferSize];
			DWORD pathsLength = 0;

			auto result = GetVolumePathNamesForVolumeNameW(buffer, pathBuffer, bufferSize, &pathsLength);
			Assert(result != FALSE);

			DWORD i = 0;
			while (i < pathsLength)
			{
				wstring volumePath(pathBuffer + i);
				i += volumePath.length() + 1;

				if (volumePath.length() > 0)
				{
					volumes.push_back(Utf16ToUtf8(volumePath));
				}
			}
		}
		while (FindNextVolumeW(findHandle, buffer, bufferSize) != FALSE);

		FindVolumeClose(findHandle);
		SetLastError(ERROR_SUCCESS);

		sort(begin(volumes), end(volumes));
		return volumes;
	}

	bool AppendFileLengthAndReadItWholeTo(const wstring& path, string& targetBuffer)
	{
		bool succeeded = false;
		auto fileHandle = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);

		if (fileHandle == INVALID_HANDLE_VALUE)
		{
			goto finish;
		}

		LARGE_INTEGER fileSize;
		if (GetFileSizeEx(fileHandle, &fileSize) == FALSE)
		{
			goto finish;
		}

		// Max 64 MB files supported at this time
		if (fileSize.QuadPart > 64 * 1024 * 1024)
		{
			SetLastError(ERROR_FILE_TOO_LARGE);
			goto finish;
		}

		targetBuffer += to_string(fileSize.QuadPart);
		targetBuffer += "\r\n\r\n";
		
		auto bufferSizeBefore = targetBuffer.size();
		targetBuffer.resize(bufferSizeBefore + static_cast<unsigned int>(fileSize.QuadPart));

		DWORD numberOfBytesRead;
		if (ReadFile(fileHandle, &targetBuffer[0] + bufferSizeBefore, static_cast<DWORD>(fileSize.QuadPart), &numberOfBytesRead, nullptr) == FALSE)
		{
			goto finish;
		}

		if (numberOfBytesRead != fileSize.QuadPart)
		{
			goto finish;
		}

		succeeded = true;

	finish:

		if (fileHandle != INVALID_HANDLE_VALUE)
		{
			CloseHandle(fileHandle);
		}
		
		return succeeded;
	}
}