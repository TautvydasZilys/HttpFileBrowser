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

	s_OutputFile = CreateFile2(kLogFileName, FILE_GENERIC_WRITE, FILE_SHARE_READ, openMode, nullptr);
	Assert(s_OutputFile != INVALID_HANDLE_VALUE || !forceOverwrite);

	if (s_OutputFile == INVALID_HANDLE_VALUE)
	{
		Assert(GetLastError() == ERROR_FILE_EXISTS);

		s_OutputFile = CreateFile2(kLogFileName, FILE_GENERIC_WRITE, FILE_SHARE_READ, OPEN_EXISTING, nullptr);
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
	Assert(strLength < static_cast<size_t>(std::numeric_limits<int>::max()));
	Assert(destinationLength < static_cast<size_t>(std::numeric_limits<int>::max()));

	auto length = MultiByteToWideChar(CP_UTF8, 0, str, static_cast<int>(strLength), destination, static_cast<int>(destinationLength));
	Assert(length > 0);

	Assert(static_cast<size_t>(length) < destinationLength);
	destination[length] = '\0';

	return length;
}

wstring Encoding::Utf8ToUtf16(const char* str, size_t strLength)
{
	if (strLength == 0) return wstring();

	auto bufferSize = 4 * strLength;
	std::unique_ptr<wchar_t[]> buffer(new wchar_t[bufferSize]);

	Assert(strLength < static_cast<size_t>(std::numeric_limits<int>::max()));
	Assert(bufferSize < static_cast<size_t>(std::numeric_limits<int>::max()));

	auto length = Utf8ToUtf16Inline(str, strLength, buffer.get(), bufferSize);
	return wstring(buffer.get(), length);
}

size_t Encoding::Utf16ToUtf8Inline(const wchar_t* wstr, size_t wstrLength, char* destination, size_t destinationLength)
{
	Assert(destinationLength >= wstrLength);
	Assert(wstrLength < static_cast<size_t>(std::numeric_limits<int>::max()));
	Assert(destinationLength < static_cast<size_t>(std::numeric_limits<int>::max()));

	auto length = WideCharToMultiByte(CP_UTF8, 0, wstr, static_cast<int>(wstrLength), destination, static_cast<int>(destinationLength), nullptr, nullptr);
	Assert(length > 0);

	Assert(static_cast<size_t>(length) < destinationLength);
	destination[length] = '\0';

	return length;
}

string Encoding::Utf16ToUtf8(const wchar_t* wstr, size_t wstrLength)
{
	if (wstrLength == 0) return string();

	auto bufferSize = 8 * wstrLength;
	std::unique_ptr<char[]> buffer(new char[bufferSize]);

	auto length = Utf16ToUtf8Inline(wstr, wstrLength, buffer.get(), bufferSize);
	return string(buffer.get(), length);
}

static char HexCharToNumber(char c)
{
	if (c > '9')
	{
		if (c > 'Z')
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

static char HexDigitToHexChar(uint8_t digit)
{
	Assert(digit >= 0 && digit < 16);

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

	size_t encodedLength = 0;
	bool needsEncoding = false;

	for (auto& c : url)
	{
		if (!NeedsUrlEncoding(c))
		{
			encodedLength++;
		}
		else
		{
			encodedLength += 3;
			needsEncoding = true;
		}
	}

	if (encodedLength == url.length())
	{
		return;
	}

	int index = static_cast<int>(url.length() - 1);
	auto encodedIndex = encodedLength - 1;
	url.resize(encodedLength);

	for (; index > -1; index--)
	{
		auto c = url[index];
		
		if (!NeedsUrlEncoding(c))
		{
			url[encodedIndex--] = c;
		}
		else
		{
			url[encodedIndex--] = HexDigitToHexChar(static_cast<uint8_t>(c) & 0xf);
			url[encodedIndex--] = HexDigitToHexChar(static_cast<uint8_t>(c) >> 4);
			url[encodedIndex--] = '%';
		}
	}
}

static const char s_Base64Table[64] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 
										  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 
										  'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 
										  'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/' };

void Encoding::EncodeBase64Inline(std::string& data)
{
	Assert(data.length() < static_cast<size_t>(std::numeric_limits<int>::max()));
	int dataLength = static_cast<int>(data.length());
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

	Assert(path.length() - 2 < static_cast<size_t>(std::numeric_limits<int>::max()));
	int i = static_cast<int>(path.length() - 2);

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
	else
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
	WIN32_FILE_ATTRIBUTE_DATA fileAttributes;
	auto result = GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &fileAttributes);

	if (result == FALSE)
	{
		auto lastError = GetLastError();
		SetLastError(ERROR_SUCCESS);

		return lastError == ERROR_FILE_NOT_FOUND ? FileStatus::FileNotFound : FileStatus::AccessDenied;
	}

	return (fileAttributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 ? FileStatus::Directory : FileStatus::File;
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
	auto findHandle = FindFirstFileExW(path.c_str(), FindExInfoBasic, &findData, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);

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
			return _stricmp(left.fileName.c_str(), right.fileName.c_str()) < 0;
		}

		return left.fileStatus == FileStatus::Directory;
	});

	return result;
}

vector<string> FileSystem::EnumerateSystemVolumes()
{
	vector<string> volumes;

#if !PHONE
	using namespace Encoding;

	const int bufferSize = 256;
	wchar_t buffer[bufferSize];

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
			i += static_cast<DWORD>(volumePath.length()) + 1;

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
#else
	// Guaranteed to exist...
	volumes.push_back("C:\\Windows");
#endif

	return volumes;
}

bool FileSystem::GetFileSizeFromHandle(HANDLE fileHandle, uint64_t& fileSize)
{
	FILE_STANDARD_INFO fileInfo;
	
	if (GetFileInformationByHandleEx(fileHandle, FileStandardInfo, &fileInfo, sizeof(fileInfo) == FALSE))
	{
		return false;
	}

	fileSize = fileInfo.EndOfFile.QuadPart;
	return true;
}

vector<uint8_t> FileSystem::ReadFileToVector(const std::wstring& path)
{
	using namespace Utilities;

	auto fileHandle = CreateFile2(path.c_str(), GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, nullptr);

	if (fileHandle == INVALID_HANDLE_VALUE)
	{
		Logging::FatalError(GetLastError(), "Failed to open \"", Encoding::Utf16ToUtf8(path), "\": ");
	}

	uint64_t fileSize;
	if (!GetFileSizeFromHandle(fileHandle, fileSize))
	{
		Logging::FatalError(GetLastError(), "Failed to get size of \"", Encoding::Utf16ToUtf8(path), "\": ");
	}

	if (fileSize > 64 * 1024 * 1024)
	{
		Logging::FatalError(ERROR_FILE_TOO_LARGE, "\"", Encoding::Utf16ToUtf8(path), "\": ");
	}

	vector<uint8_t> fileBytes(static_cast<size_t>(fileSize));

	if (fileSize > 0)
	{
		DWORD numberOfBytesRead;
		if (ReadFile(fileHandle, &fileBytes[0], static_cast<DWORD>(fileSize), &numberOfBytesRead, nullptr) == FALSE ||
			numberOfBytesRead != fileSize)
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
#if !PHONE
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
#else
	return "implementMePls";
#endif
}

static string GetUniqueSystemIdImpl()
{
	auto macAddress = GetMacAddress();
	Encoding::EncodeBase64Inline(macAddress);
	return macAddress;
}

const string& System::GetUniqueSystemId()
{
	static string s_UniqueSystemId = GetUniqueSystemIdImpl();
	return s_UniqueSystemId;
}

void System::Sleep(int milliseconds)
{
#if !PHONE
	::Sleep(milliseconds);
#else
	auto ev = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_MODIFY_STATE);
	WaitForSingleObjectEx(ev, milliseconds, FALSE);
	CloseHandle(ev);
#endif
}

/*
* Copyright (c) 1996,1999 by Internet Software Consortium.
*
* Permission to use, copy, modify, and distribute this software for any
* purpose with or without fee is hereby granted, provided that the above
* copyright notice and this permission notice appear in all copies.
*
* THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
* ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
* CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
* DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
* PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
* ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
* SOFTWARE.
*/

/*
 * WARNING: Don't even consider trying to compile this on a system where
 * sizeof(int) < 4.  sizeof(int) > 4 is fine; all the world's not a VAX.
 */

static int inet_pton4(const char *src, u_char *dst);
static int inet_pton6(const char *src, u_char *dst);

/* int
* inet_pton(af, src, dst)
*	convert from presentation format (which usually means ASCII printable)
*	to network format (which is usually some kind of binary format).
* return:
*	1 if the address was valid for the specified address family
*	0 if the address wasn't valid (`dst' is untouched in this case)
*	-1 if some other error occurred (`dst' is untouched in this case, too)
* author:
*	Paul Vixie, 1996.
*/
int Encoding::inet_pton(int af, const char* src, void* dst)
{
	switch (af) {
	case AF_INET:
		return (inet_pton4(src, static_cast<u_char*>(dst)));
	case AF_INET6:
		return (inet_pton6(src, static_cast<u_char*>(dst)));
	default:
		Assert(false);
		return (-1);
	}
	/* NOTREACHED */
}

/* int
* inet_pton4(src, dst)
*	like inet_aton() but without all the hexadecimal, octal (with the
*	exception of 0) and shorthand.
* return:
*	1 if `src' is a valid dotted quad, else 0.
* notice:
*	does not touch `dst' unless it's returning 1.
* author:
*	Paul Vixie, 1996.
*/
static int inet_pton4(const char *src, u_char *dst)
{
	const int NS_INADDRSZ = 4;
	int saw_digit, octets, ch;
	u_char tmp[NS_INADDRSZ], *tp;

	saw_digit = 0;
	octets = 0;
	*(tp = tmp) = 0;
	while ((ch = *src++) != '\0') {

		if (ch >= '0' && ch <= '9') {
			u_int newI = *tp * 10 + (ch - '0');

			if (saw_digit && *tp == 0)
				return (0);
			if (newI > 255)
				return (0);
			*tp = newI;
			if (!saw_digit) {
				if (++octets > 4)
					return (0);
				saw_digit = 1;
			}
		}
		else if (ch == '.' && saw_digit) {
			if (octets == 4)
				return (0);
			*++tp = 0;
			saw_digit = 0;
		}
		else
			return (0);
	}
	if (octets < 4)
		return (0);
	memcpy(dst, tmp, NS_INADDRSZ);
	return (1);
}

/* int
* inet_pton6(src, dst)
*	convert presentation level address to network order binary form.
* return:
*	1 if `src' is a valid [RFC1884 2.2] address, else 0.
* notice:
*	(1) does not touch `dst' unless it's returning 1.
*	(2) :: in a full address is silently ignored.
* credit:
*	inspired by Mark Andrews.
* author:
*	Paul Vixie, 1996.
*/
static int inet_pton6(const char *src, u_char *dst)
{
	static const char xdigits[] = "0123456789abcdef";
	const int NS_INT16SZ = 2;
	const int NS_INADDRSZ = 4;
	const int NS_IN6ADDRSZ = 16;

	u_char tmp[NS_IN6ADDRSZ], *tp, *endp, *colonp;
	const char *curtok;
	int ch, saw_xdigit;
	u_int val;

	tp = static_cast<u_char*>(memset(tmp, '\0', NS_IN6ADDRSZ));
	endp = tp + NS_IN6ADDRSZ;
	colonp = NULL;
	/* Leading :: requires some special handling. */
	if (*src == ':')
		if (*++src != ':')
			return (0);
	curtok = src;
	saw_xdigit = 0;
	val = 0;
	while ((ch = tolower(*src++)) != '\0') {
		const char *pch;

		pch = strchr(xdigits, ch);
		if (pch != NULL) {
			val <<= 4;
			val |= (pch - xdigits);
			if (val > 0xffff)
				return (0);
			saw_xdigit = 1;
			continue;
		}
		if (ch == ':') {
			curtok = src;
			if (!saw_xdigit) {
				if (colonp)
					return (0);
				colonp = tp;
				continue;
			}
			else if (*src == '\0') {
				return (0);
			}
			if (tp + NS_INT16SZ > endp)
				return (0);
			*tp++ = (u_char)(val >> 8) & 0xff;
			*tp++ = (u_char)val & 0xff;
			saw_xdigit = 0;
			val = 0;
			continue;
		}
		if (ch == '.' && ((tp + NS_INADDRSZ) <= endp) &&
			inet_pton4(curtok, tp) > 0) {
			tp += NS_INADDRSZ;
			saw_xdigit = 0;
			break;	/* '\0' was seen by inet_pton4(). */
		}
		return (0);
	}
	if (saw_xdigit) {
		if (tp + NS_INT16SZ > endp)
			return (0);
		*tp++ = (u_char)(val >> 8) & 0xff;
		*tp++ = (u_char)val & 0xff;
	}
	if (colonp != NULL) {
		/*
		* Since some memmove()'s erroneously fail to handle
		* overlapping regions, we'll do the shift by hand.
		*/
		auto n = tp - colonp;
		int i;

		if (tp == endp)
			return (0);
		for (i = 1; i <= n; i++) {
			endp[-i] = colonp[n - i];
			colonp[n - i] = 0;
		}
		tp = endp;
	}
	if (tp != endp)
		return (0);
	memcpy(dst, tmp, NS_IN6ADDRSZ);
	return (1);
}