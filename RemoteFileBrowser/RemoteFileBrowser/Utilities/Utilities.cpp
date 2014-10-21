#include "PrecompiledHeader.h"

using namespace std;
using namespace Utilities;

// Logging

CriticalSection Logging::s_LogCriticalSection;

static HANDLE s_OutputFile;
static const wchar_t kLogFileName[] = L"LogFile.log";

void Logging::Initialize(bool forceOverwrite)
{
	auto openMode = forceOverwrite ? CREATE_ALWAYS : CREATE_NEW;
	s_OutputFile = CreateFile(kLogFileName, FILE_GENERIC_WRITE, FILE_SHARE_READ, nullptr, openMode, FILE_ATTRIBUTE_NORMAL, nullptr);
	Assert(s_OutputFile != INVALID_HANDLE_VALUE || !forceOverwrite);

	if (s_OutputFile == INVALID_HANDLE_VALUE)
	{
		Assert(GetLastError() == ERROR_FILE_EXISTS);

		s_OutputFile = CreateFile(kLogFileName, FILE_GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		Assert(s_OutputFile != INVALID_HANDLE_VALUE);

		const char threeNewLines[] = "\r\n\r\n\r\n";
		DWORD bytesWritten;

		auto result = WriteFile(s_OutputFile, threeNewLines, sizeof(threeNewLines), &bytesWritten, nullptr);
		Assert(result != FALSE);
		Assert(bytesWritten = sizeof(threeNewLines));
	}
	else
	{
		const uint8_t utf8ByteOrderMark[] = { 0xEF, 0xBB, 0xBF };
		DWORD bytesWritten;

		auto result = WriteFile(s_OutputFile, utf8ByteOrderMark, sizeof(utf8ByteOrderMark), &bytesWritten, nullptr);
		Assert(result != FALSE);
		Assert(bytesWritten = sizeof(utf8ByteOrderMark));
	}

	SetLastError(ERROR_SUCCESS);
}

void Logging::Shutdown()
{
	CloseHandle(s_OutputFile);
}

std::wstring Logging::GetLogFileName()
{
	return kLogFileName;
}

void Logging::OutputMessage(const char* message, size_t length)
{
	if (IsDebuggerPresent())
	{
		OutputDebugStringA(message);
	}

	DWORD bytesWritten;

	auto result = WriteFile(s_OutputFile, message, static_cast<DWORD>(length), &bytesWritten, nullptr);
	Assert(result != FALSE);
	Assert(bytesWritten == length);
}

static inline size_t SystemTimeToStringInline(wchar_t (&buffer)[Logging::kBufferSize], SYSTEMTIME* systemTime = nullptr)
{
	auto dateLength = GetDateFormatEx(LOCALE_NAME_SYSTEM_DEFAULT, DATE_SHORTDATE, systemTime, nullptr, buffer, Logging::kBufferSize, nullptr);
	buffer[dateLength - 1] = ' ';
	auto timeLength = GetTimeFormatEx(LOCALE_NAME_SYSTEM_DEFAULT, 0, systemTime, nullptr, buffer + dateLength, Logging::kBufferSize - dateLength);

	return dateLength + timeLength;
}

static wstring SystemTimeToString(SYSTEMTIME* systemTime = nullptr)
{
	wchar_t buffer[Logging::kBufferSize];
	SystemTimeToStringInline(buffer, systemTime);
	return buffer;
}

void Logging::OutputCurrentTimestamp()
{
	OutputMessage("[");

	char buffer[kBufferSize];
	wchar_t wbuffer[kBufferSize];
	auto dateTimeLength = SystemTimeToStringInline(wbuffer);
	Encoding::Utf16ToUtf8Inline(wbuffer, dateTimeLength, buffer, kBufferSize);

	OutputMessage(buffer);
	OutputMessage("] ");
}

wstring Logging::Win32ErrorToMessage(int win32ErrorCode)
{
	wchar_t buffer[kBufferSize];
	Win32ErrorToMessageInline(win32ErrorCode, buffer);
	return buffer;
}

void Logging::Terminate(int errorCode)
{
	Logging::Shutdown();
	__fastfail(errorCode); // Just crash™ - let user know we crashed by bringing up WER dialog
}

// Encoding

size_t Encoding::Utf8ToUtf16Inline(const char* str, size_t strLength, wchar_t* destination, size_t destinationLength)
{
	Assert(destinationLength >= strLength);

	auto length = MultiByteToWideChar(CP_UTF8, 0, str, strLength, destination, destinationLength);
	Assert(length > 0);

	return length;
}

wstring Encoding::Utf8ToUtf16(const char* str, size_t strLength)
{
	if (strLength == 0) return wstring();

	auto bufferSize = 4 * strLength;
	std::unique_ptr<wchar_t[]> buffer(new wchar_t[bufferSize]);

	auto length = MultiByteToWideChar(CP_UTF8, 0, str, strLength, buffer.get(), bufferSize);
	Assert(length > 0);

	return wstring(buffer.get(), length);
}

size_t Encoding::Utf16ToUtf8Inline(const wchar_t* wstr, size_t wstrLength, char* destination, size_t destinationLength)
{
	Assert(destinationLength >= wstrLength);

	auto length = WideCharToMultiByte(CP_UTF8, 0, wstr, wstrLength, destination, destinationLength, nullptr, nullptr);
	Assert(length > 0);

	return length;
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

static char s_Base64Table1[64] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 
								  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 
								  'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 
								  'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '9', '+', '/' };

static vector<char> s_Base64Table(s_Base64Table1, s_Base64Table1 + 64);

void Encoding::EncodeBase64Inline(std::string& data)
{
	int dataLength = data.length();
	if (dataLength == 0) return;

	int remainder = dataLength % 3;
	int blockCount = dataLength / 3 + (remainder == 0 ? 0 : 1);
	data.resize(4 * blockCount);

	int i;

	// Handle padding
	if (remainder > 0)
	{
		auto lastByte = static_cast<uint8_t>(data[dataLength - 1]);

		if (remainder == 1)
		{
			// bbbbbb'bb 0000'0000 00'000000
			data[data.length() - 1] = '=';
			data[data.length() - 2] = '=';
			data[data.length() - 3] = s_Base64Table[lastByte & 0x03];
			data[data.length() - 4] = s_Base64Table[(lastByte & 0xFC) >> 2];
		}
		else if (remainder == 2)
		{
			// bbbbbb'bb bbbb'bbbb 00'000000
			auto preLastByte = static_cast<uint8_t>(data[dataLength - 2]);

			data[data.length() - 1] = '=';
			data[data.length() - 2] = s_Base64Table[(lastByte & 0x0F) << 2];
			data[data.length() - 3] = s_Base64Table[((preLastByte & 0x03) << 4) | ((lastByte & 0xF0) >> 4)];
			data[data.length() - 4] = s_Base64Table[(preLastByte & 0xFC) >> 2];
		}

		i = blockCount - 2;
	}
	else
	{
		i = blockCount - 1;
	}

	// Encode main blocks
	for (; i > -1; i--)
	{
		//     a         b         c
		// bbbbbb'bb bbbb'bbbb bb'bbbbbb

		auto dataIndex = 3 * i;
		auto resultIndex = 4 * i;
		auto a = static_cast<uint8_t>(data[dataIndex]);
		auto b = static_cast<uint8_t>(data[dataIndex + 1]);
		auto c = static_cast<uint8_t>(data[dataIndex + 2]);

		data[resultIndex] = s_Base64Table[(a & 0xFC) >> 2];
		data[resultIndex + 1] = s_Base64Table[((a & 0x03) << 4) | ((b & 0xF0) >> 4)];
		data[resultIndex + 2] = s_Base64Table[((b & 0x0F) << 2) | ((c & 0xC0) >> 6)];
		data[resultIndex + 3] = s_Base64Table[c & 0x3F];
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
		Logging::FatalError(GetLastError(), "Failed to open \"", Encoding::Utf16ToUtf8(path), "\": ");
	}

	LARGE_INTEGER fileSize;
	if (GetFileSizeEx(fileHandle, &fileSize) == FALSE)
	{
		Logging::FatalError(GetLastError(), "Failed to get size of \"", Encoding::Utf16ToUtf8(path), "\": ");
	}

	if (fileSize.QuadPart > 64 * 1024 * 1024)
	{
		Logging::FatalError(ERROR_FILE_TOO_LARGE, "\"", Encoding::Utf16ToUtf8(path), "\": ");
	}

	vector<uint8_t> fileBytes(static_cast<size_t>(fileSize.QuadPart));

	if (fileSize.QuadPart > 0)
	{
		DWORD numberOfBytesRead;
		if (ReadFile(fileHandle, &fileBytes[0], static_cast<DWORD>(fileSize.QuadPart), &numberOfBytesRead, nullptr) == FALSE ||
			numberOfBytesRead != fileSize.QuadPart)
		{
			Logging::FatalError(GetLastError(), "Failed to read \"", Encoding::Utf16ToUtf8(path), "\": ");
		}
	}

	CloseHandle(fileHandle);
	return fileBytes;
}


// System

static string GetMacAddress()
{
	ULONG infoSize = 0;

	auto result = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_SKIP_FRIENDLY_NAME, nullptr, nullptr, &infoSize);
	Logging::LogFatalErrorIfFailed(result != ERROR_BUFFER_OVERFLOW, "Failed to get memory size needed for adapters addresses: ");

	unique_ptr<uint8_t[]> adapterAddressBuffer(new uint8_t[infoSize]);
	auto adapterAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(adapterAddressBuffer.get());

	result = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_SKIP_FRIENDLY_NAME, nullptr, adapterAddresses, &infoSize);
	Logging::LogFatalErrorIfFailed(result != ERROR_SUCCESS, "Failed to get adapters addresses: ");

	auto macLength = adapterAddresses->PhysicalAddressLength;
	string macAddress;

	macAddress.resize(macLength);
	memcpy(&macAddress[0], adapterAddresses->PhysicalAddress, macLength);

	return macAddress;
}

static string GetUniqueSystemIdImpl()
{
	auto macAddress = GetMacAddress();
	Encoding::EncodeBase64Inline(macAddress);
	return macAddress;
}

const string& Utilities::System::GetUniqueSystemId()
{
	static string s_UniqueSystemId = GetUniqueSystemIdImpl();
	return s_UniqueSystemId;
}
