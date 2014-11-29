#include "PrecompiledHeader.h"

#define EXPORT extern "C" __declspec(dllexport)

enum class FileType
{
	File,
	Directory
};

struct SimpleFileInfo
{
	FileType fileType;
	wchar_t* fileName;
};

EXPORT void __stdcall GetFilesInDirectory(const wchar_t* directoryName, SimpleFileInfo*& results, int& resultCount)
{
	auto files = Utilities::FileSystem::EnumerateFiles(directoryName);
	Assert(files.size() < std::numeric_limits<int>::max() && static_cast<int>(files.size()) > -1);

	resultCount = static_cast<int>(files.size());
	results = new SimpleFileInfo[resultCount];

	for (int i = 0; i < resultCount; i++)
	{
		const size_t kBufferSize = 512;
		wchar_t buffer[kBufferSize];

		auto length = Utilities::Encoding::Utf8ToUtf16Inline(files[i].fileName, buffer, kBufferSize);
		results[i].fileName = new wchar_t[length + 1];
		memcpy(results[i].fileName, buffer, (length + 1)* sizeof(wchar_t));

		results[i].fileType = files[i].fileStatus == Utilities::FileSystem::FileStatus::Directory ? FileType::Directory : FileType::File;
	}
}

EXPORT void __stdcall FreeFileData(SimpleFileInfo* files, int fileCount)
{
	for (int i = 0; i < fileCount; i++)
	{
		delete[] files[i].fileName;
	}

	delete[] files;
}