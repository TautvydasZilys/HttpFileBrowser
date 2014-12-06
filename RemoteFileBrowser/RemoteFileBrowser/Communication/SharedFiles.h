#pragma once

namespace SharedFiles
{
	typedef std::unordered_set<std::string, Utilities::String::CaseInsensitiveHasher, Utilities::String::CaseInsensitiveComparer> FileSet;

	void SetSharedFiles(FileSet&& fullySharedFolders, FileSet&& partiallySharedFolders, FileSet&& files);
	bool IsFileShared(const std::string& path);
	bool IsFolderVisible(const std::string& path);
	std::vector<Utilities::FileSystem::FileInfo> GetFolderContents(const std::string& path);
};

