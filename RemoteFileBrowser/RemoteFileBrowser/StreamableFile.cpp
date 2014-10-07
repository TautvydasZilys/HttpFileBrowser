#include "PrecompiledHeader.h"
#include "StreamableFile.h"

using namespace std;

StreamableFile::StreamableFile(const std::wstring& filePath) :
	m_FilePosition(0)
{
	bool succeeded = false;
	m_FileHandle = CreateFileW(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);

	if (m_FileHandle == INVALID_HANDLE_VALUE)
	{
		throw exception();
	}

	if (GetFileSizeEx(m_FileHandle, reinterpret_cast<LARGE_INTEGER*>(&m_FileSize)) == FALSE)
	{
		CloseHandle(m_FileHandle);
		throw exception();
	}
}

StreamableFile::~StreamableFile()
{
	CloseHandle(m_FileHandle);
}

void StreamableFile::ReadNextChunk(char* buffer, int& bytesRead)
{
	DWORD numberOfBytesToRead = static_cast<DWORD>(min(m_FileSize - m_FilePosition, kMaxChunkSize));

	if (ReadFile(m_FileHandle, buffer, numberOfBytesToRead, reinterpret_cast<DWORD*>(&bytesRead), nullptr) == FALSE ||
		bytesRead != numberOfBytesToRead)
	{
		throw exception();
	}

	m_FilePosition += bytesRead;
}