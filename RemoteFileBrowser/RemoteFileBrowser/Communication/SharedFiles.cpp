#include "PrecompiledHeader.h"
#include "SharedFiles.h"
#include "Utilities\CriticalSection.h"

static SharedFiles::FileSet s_FullySharedFolders;
static SharedFiles::FileSet s_PartiallySharedFolders;
static SharedFiles::FileSet s_Files;
static CriticalSection s_CriticalSection;

void SharedFiles::SetSharedFiles(FileSet&& fullySharedFolders, FileSet&& partiallySharedFolders, FileSet&& files)
{
	CriticalSection::Lock lock(s_CriticalSection);

	s_FullySharedFolders = fullySharedFolders;
	s_PartiallySharedFolders = partiallySharedFolders;
	s_Files = files;
}

namespace NoLock
{
	static inline bool IsFolderFullyShared(std::string&& path)
	{
		while (!path.empty())
		{
			if (s_FullySharedFolders.find(path) != s_FullySharedFolders.end())
				return true;

			Utilities::FileSystem::RemoveLastPathComponentInline(path);
		}

		return false;
	}

	static inline bool IsFolderFullyShared(const std::string& path)
	{
		if (s_FullySharedFolders.find(path) != s_FullySharedFolders.end())
			return true;

		return IsFolderFullyShared(Utilities::FileSystem::RemoveLastPathComponent(path));
	}

	static inline bool IsFileShared(const std::string& path)
	{
		if (s_Files.find(path) != s_Files.end())
			return true;

		return IsFolderFullyShared(Utilities::FileSystem::RemoveLastPathComponent(path));
	}

	static inline bool IsFolderVisible(const std::string& path)
	{
		if (s_PartiallySharedFolders.find(path) != s_PartiallySharedFolders.end())
			return true;

		return IsFolderFullyShared(path);
	}

	static inline void FilterFolderContents(const std::string& basePath, std::vector<Utilities::FileSystem::FileInfo>& folderContents)
	{
		using namespace Utilities::FileSystem;

		Utilities::Algorithms::FilterVector(folderContents, [&basePath](const FileInfo& fileInfo)
		{
			if (fileInfo.fileStatus == FileStatus::Directory)
				return IsFolderVisible(CombinePaths(basePath, fileInfo.fileName));
		
			return IsFileShared(CombinePaths(basePath, fileInfo.fileName));
		});
	}
}

bool SharedFiles::IsFileShared(const std::string& path)
{
	CriticalSection::Lock lock(s_CriticalSection);
	return NoLock::IsFileShared(path);
}

bool SharedFiles::IsFolderVisible(const std::string& path)
{
	CriticalSection::Lock lock(s_CriticalSection);
	return NoLock::IsFolderVisible(path);
}

std::vector<Utilities::FileSystem::FileInfo> SharedFiles::GetFolderContents(const std::string& path)
{
	using namespace Utilities::FileSystem;
	CriticalSection::Lock lock(s_CriticalSection);

	auto folderContents = EnumerateFiles(Utilities::Encoding::Utf8ToUtf16(path));

	if (!NoLock::IsFolderFullyShared(path))
		NoLock::FilterFolderContents(path, folderContents);

	SortFiles(folderContents);
	return folderContents;
}