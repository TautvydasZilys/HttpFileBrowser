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

struct IconInfo
{
	HICON icon;
	int width;
	int height;
};

EXPORT void __stdcall GetIcon(const wchar_t* path, IconInfo& iconData)
{
	SHFILEINFOW fileInfo;

	auto getFileInfoResult = SHGetFileInfoW(path, 0, &fileInfo, sizeof(fileInfo), SHGFI_ICON | SHGFI_SHELLICONSIZE);
	Assert(getFileInfoResult != 0);

	ICONINFO iconInfo;
	auto getIconInfoResult = GetIconInfo(fileInfo.hIcon, &iconInfo);
	Assert(getIconInfoResult != FALSE);

	BITMAP bitmap = { 0 };

	if (iconInfo.hbmColor != nullptr)
	{
		auto result = GetObject(iconInfo.hbmColor, sizeof(bitmap), &bitmap);
		Assert(result > 0);
		DeleteObject(iconInfo.hbmColor);
	}
	else if (iconInfo.hbmMask != nullptr)
	{
		auto result = GetObject(iconInfo.hbmMask, sizeof(bitmap), &bitmap);
		Assert(result > 0);
	}

	if (iconInfo.hbmMask != nullptr)
	{
		DeleteObject(iconInfo.hbmMask);
	}

	iconData.icon = fileInfo.hIcon;
	iconData.width = bitmap.bmWidth;
	iconData.height = bitmap.bmHeight;
}