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

	wstring GetCurrentTimestamp()
	{
		const int bufferSize = 256;
		wchar_t buffer[bufferSize];

		auto dateLength = GetDateFormatEx(LOCALE_NAME_SYSTEM_DEFAULT, DATE_SHORTDATE, nullptr, nullptr, buffer + 1, bufferSize - 1, nullptr);		
		buffer[dateLength] = ' ';
		auto timeLength = GetTimeFormatEx(LOCALE_NAME_SYSTEM_DEFAULT, 0, nullptr, nullptr, buffer + dateLength + 1, bufferSize - dateLength - 1);

		buffer[0] = '[';
		buffer[dateLength + timeLength] = ']';
		buffer[dateLength + timeLength + 1] = ' ';
		buffer[dateLength + timeLength + 2] = '\0';

		return buffer;
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

		sort(begin(volumes), end(volumes));
		return volumes;
	}
}