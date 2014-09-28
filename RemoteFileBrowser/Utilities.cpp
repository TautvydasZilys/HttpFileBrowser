#include "PrecompiledHeader.h"

using namespace std;
using namespace Utilities;

// Logging

mutex Logging::s_LogMutex;

static wofstream s_OutputFile;
static const wchar_t kLogFileName[] = L"LogFile.log";

void Logging::OutputMessage(const wchar_t* message)
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

static inline void SystemTimeToStringInline(wchar_t (&buffer)[Logging::kBufferSize], SYSTEMTIME* systemTime = nullptr)
{
	auto dateLength = GetDateFormatEx(LOCALE_NAME_SYSTEM_DEFAULT, DATE_SHORTDATE, systemTime, nullptr, buffer, Logging::kBufferSize, nullptr);
	buffer[dateLength - 1] = ' ';
	auto timeLength = GetTimeFormatEx(LOCALE_NAME_SYSTEM_DEFAULT, 0, systemTime, nullptr, buffer + dateLength, Logging::kBufferSize - dateLength);
}

static wstring SystemTimeToString(SYSTEMTIME* systemTime = nullptr)
{
	wchar_t buffer[Logging::kBufferSize];
	SystemTimeToStringInline(buffer, systemTime);
	return buffer;
}

void Logging::OutputCurrentTimestamp()
{
	OutputMessage(L"[");

	wchar_t buffer[kBufferSize];
	SystemTimeToStringInline(buffer);
	OutputMessage(buffer);

	OutputMessage(L"] ");
}

wstring Logging::Win32ErrorToMessage(int win32ErrorCode)
{
	wchar_t buffer[kBufferSize];
	Win32ErrorToMessageInline(win32ErrorCode, buffer);
	return buffer;
}

void Logging::Terminate(int errorCode)
{
	if (s_OutputFile.is_open())
	{
		s_OutputFile.close();
	}

	__fastfail(errorCode); // Just crash™ - let user know we crashed by bringing up WER dialog
}

// Encoding

wstring Encoding::Utf8ToUtf16(const char* str, size_t strLength)
{
	if (strLength == 0) return wstring();

	auto bufferSize = 4 * strLength;
	std::unique_ptr<wchar_t[]> buffer(new wchar_t[bufferSize]);

	auto length = MultiByteToWideChar(CP_UTF8, 0, str, strLength, buffer.get(), bufferSize);
	Assert(length > 0);

	return wstring(buffer.get(), length);
}

string Encoding::Utf16ToUtf8(const wchar_t* wstr, size_t wstrLength)
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

static char HexDigitToHexChar(int digit)
{
	if (digit < 10)
	{
		return digit + '0';
	}
	else
	{
		return digit + 'A' - 10;
	}
}

void Encoding::DecodeUrlInline(string& url)
{
	auto urlLength = url.length();
	unsigned int i = 0;
	unsigned int decodedLength = 0;
		
	while (i < urlLength)
	{
		if (url[i] == '+')
		{
			url[decodedLength++] = ' ';
			i++;
			continue;
		}
		else if (url[i] != '%')
		{
			url[decodedLength++] = url[i];
			i++;
			continue;
		}

		if (urlLength - i < 3)
		{
			break;
		}

		url[decodedLength++] = static_cast<char>(HexCharToNumber(url[i + 1]) * 16 + HexCharToNumber(url[i + 2]));
		i += 3;
	}

	url.resize(decodedLength);
}

static bool NeedsUrlEncoding(char c)
{
	if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
	{
		return false;
	}

	switch (c)
	{
	case '-':
	case '_':
	case '.':
	case '!':
	case '*':
	case '\'':
	case '(':
	case ')':
		return false;
	}

	return true;
}

void Encoding::EncodeUrlInline(string& url)
{
	// First count resulting string length, then encode

	string encoded;
	int encodedLength = 0;
	bool needsEncoding = false;

	for (auto& c : url)
	{
		if (c == ' ')
		{
			encodedLength++;
			needsEncoding = true;
		}
		else if (!NeedsUrlEncoding(c))
		{
			encodedLength++;
		}
		else
		{
			encodedLength += 3;
			needsEncoding = true;
		}
	}

	if (!needsEncoding)
	{
		return;
	}

	if (encodedLength != url.length())
	{
		url.reserve(encodedLength);
	}

	int encodedIndex = encodedLength - 1;

	for (unsigned int i = url.length() - 1; i > -1; i--)
	{
		auto c = url[i];

		if (c == ' ')
		{
			url[encodedIndex--] += '+';
		}
		else if (!NeedsUrlEncoding(c))
		{
			url[encodedIndex--] = c;
		}
		else
		{
			url[encodedIndex--] = HexDigitToHexChar(c & 0xf);
			url[encodedIndex--] = '%';
			url[encodedIndex--] = HexDigitToHexChar(c >> 4);
		}
	}
}


// File system

void FileSystem::RemoveLastPathComponentInline(string& path)
{
	if (path.length() < 2)
	{
		return;
	}

	int i = path.length() - 2;

	while (i > -1 && (path[i] != '\\' && path[i] != '/'))
	{
		i--;
	}

	path.resize(i + 1);
}
	
string FileSystem::CombinePaths(const string& left, const string& right)
{
	if (right == ".")
	{
		return left;
	}
	else if (right == "..")
	{
		return RemoveLastPathComponent(left);
	}

	auto leftLastChar = left[left.length() - 1];

	if (leftLastChar == '\\' || leftLastChar == '/')
	{
		return left + right;
	}

	string combined;		
	combined.reserve(left.length() + right.length() + 1);

	combined.append(left);
	combined.append("\\", 1);
	combined.append(right);

	return combined;
}

string FileSystem::FormatFileSizeString(uint64_t size)
{
	stringstream result;
	result.setf(ios::fixed);
	result.precision(3);

	if (size < 1024)
	{
		result << size << " B";
	}
	else if (size < 1024 * 1024)
	{
		result << static_cast<float>(size) / 1024 << " KB";
	}
	else if (size < 1024 * 1024 * 1024)
	{
		result << static_cast<float>(size / 1024) / 1024 << " MB";
	}
	else if (size < static_cast<uint64_t>(1024 * 1024 * 1024) * 1024)
	{
		result << static_cast<float>(size / 1024 / 1024) / 1024 << " GB";
	}
	else if (size < static_cast<uint64_t>(1024 * 1024 * 1024) * 1024 * 1024)
	{
		result << static_cast<float>(size / 1024 / 1024 / 1024) / 1024 << " TB";
	}

	return result.str();
}

FileSystem::FileInfo::FileInfo(const string& fileName, FileStatus fileStatus, const string& dateModified, uint64_t fileSize) :
	fileName(std::move(fileName)), fileStatus(fileStatus), dateModified(std::move(dateModified)), fileSize(fileSize)
{
}

FileSystem::FileInfo::FileInfo(string&& fileName, FileStatus fileStatus, string&& dateModified, uint64_t fileSize) :
	fileName(std::move(fileName)), fileStatus(fileStatus), dateModified(std::move(dateModified)), fileSize(fileSize)
{
}

FileSystem::FileStatus FileSystem::QueryFileStatus(const wstring& path)
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

vector<FileSystem::FileInfo> FileSystem::EnumerateFiles(wstring path)
{
	using namespace Encoding;
	using namespace FileSystem;

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
		auto fileSize = (static_cast<uint64_t>(findData.nFileSizeHigh) << 32) | findData.nFileSizeLow;

		result.emplace_back(std::move(fileName), fileStatus, std::move(dateModified), fileSize);
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

vector<string> FileSystem::EnumerateSystemVolumes()
{
	using namespace Encoding;

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

vector<uint8_t> FileSystem::ReadFileToVector(const std::wstring& path)
{
	using namespace Utilities;

	auto fileHandle = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);

	if (fileHandle == INVALID_HANDLE_VALUE)
	{
		Logging::FatalError(GetLastError(), L"Failed to open \"", path, L"\": ");
	}

	LARGE_INTEGER fileSize;
	if (GetFileSizeEx(fileHandle, &fileSize) == FALSE)
	{
		Logging::FatalError(GetLastError(), L"Failed to get size of \"", path, L"\": ");
	}

	if (fileSize.QuadPart > 64 * 1024 * 1024)
	{
		Logging::FatalError(ERROR_FILE_TOO_LARGE, L"\"", path, L"\": ");
	}

	vector<uint8_t> fileBytes(static_cast<size_t>(fileSize.QuadPart));

	if (fileSize.QuadPart > 0)
	{
		DWORD numberOfBytesRead;
		if (ReadFile(fileHandle, &fileBytes[0], static_cast<DWORD>(fileSize.QuadPart), &numberOfBytesRead, nullptr) == FALSE ||
			numberOfBytesRead != fileSize.QuadPart)
		{
			Logging::FatalError(GetLastError(), L"Failed to read \"", path, L"\": ");
		}
	}

	CloseHandle(fileHandle);
	return fileBytes;
}