#include "PrecompiledHeader.h"
#include "Communication\FileBrowserResponseHandler.h"
#include "Communication\SharedFiles.h"
#include "Http\Server.h"
#include "Tcp\Listener.h"
#include "Utilities\Initializer.h"

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
	auto files = Utilities::FileSystem::EnumerateAndSortFiles(directoryName);
	Assert(files.size() < static_cast<size_t>(std::numeric_limits<int>::max()) && static_cast<int>(files.size()) > -1);

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

EXPORT void __stdcall GetVolumes(wchar_t**& results, int& resultCount)
{
	auto volumes = Utilities::FileSystem::EnumerateSystemVolumes();
	resultCount = static_cast<int>(volumes.size());
	results = new wchar_t*[resultCount];

	for (int i = 0; i < resultCount; i++)
	{
		results[i] = new wchar_t[volumes[i].length() + 1];
		Utilities::Encoding::Utf8ToUtf16Inline(volumes[i], results[i], volumes[i].length() + 1);
	}
}

EXPORT void __stdcall FreeVolumes(wchar_t** volumes, int volumeCount)
{
	for (int i = 0; i < volumeCount; i++)
	{
		delete[] volumes[i];
	}

	delete[] volumes;
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

EXPORT void __stdcall GetUniqueSystemId(const char*& uniqueSystemIdPtr, int& length)
{
	const auto& uniqueSystemId = Utilities::System::GetUniqueSystemId();

	uniqueSystemIdPtr = uniqueSystemId.c_str();
	length = uniqueSystemId.length();
}

struct SharedFilesInterop
{
	wchar_t** fullySharedFolders;
	int fullySharedFolderCount;

	wchar_t** partiallySharedFolders;
	int partiallySharedFolderCount;

	wchar_t** sharedFiles;
	int sharedFileCount;
};

static inline void InsertToFileSet(SharedFiles::FileSet& fileSet, const wchar_t* wstr)
{
	auto str = Utilities::Encoding::Utf16ToUtf8(wstr, wcslen(wstr));
	fileSet.insert(std::move(str));
}

EXPORT void __stdcall SetSharedFiles(const SharedFilesInterop& sharedFiles)
{
	SharedFiles::FileSet fullySharedFolders, partiallySharedFolders, files;

	for (int i = 0; i < sharedFiles.fullySharedFolderCount; i++)
	{
		InsertToFileSet(fullySharedFolders, sharedFiles.fullySharedFolders[i]);
	}

	for (int i = 0; i < sharedFiles.partiallySharedFolderCount; i++)
	{
		InsertToFileSet(partiallySharedFolders, sharedFiles.partiallySharedFolders[i]);
	}

	for (int i = 0; i < sharedFiles.sharedFileCount; i++)
	{
		InsertToFileSet(files, sharedFiles.sharedFiles[i]);
	}

	SharedFiles::SetSharedFiles(std::move(fullySharedFolders), std::move(partiallySharedFolders), std::move(files));
}

static void InitializeLazy()
{
	static Initializer initializerContext;
}

EXPORT Tcp::Listener* __stdcall StartSharingFiles(const SharedFilesInterop& sharedFiles)
{
	InitializeLazy();
	SetSharedFiles(sharedFiles);

	auto listener = new Tcp::Listener(true);
	in6_addr anyAddress = { 0 };

	listener->RunAsync(anyAddress, htons(18882), [](SOCKET incomingSocket, sockaddr_in6 clientAddress)
	{
		Http::Server::StartServiceClient(incomingSocket, clientAddress, &FileBrowserResponseHandler::ExecuteRequest);
	});

	return listener;
}

EXPORT void __stdcall StopSharingFiles(Tcp::Listener*& listener)
{
	delete listener;
	listener = nullptr;
}