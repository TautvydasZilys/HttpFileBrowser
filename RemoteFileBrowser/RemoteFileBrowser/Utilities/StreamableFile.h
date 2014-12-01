#pragma once

class StreamableFile
{
private:
	HANDLE m_FileHandle;
	uint64_t m_FileSize;
	uint64_t m_FilePosition;

public:
	StreamableFile(const std::wstring& filePath);
	~StreamableFile();

	static const uint64_t kMaxChunkSize = 8 * 1024 * 1024; // 8 MB at a time

	bool IsEndOfFile() const { return m_FilePosition == m_FileSize; }
	inline uint64_t GetFileSize() const { return m_FileSize; }
	void ReadNextChunk(char* buffer, int& bytesRead);
};

