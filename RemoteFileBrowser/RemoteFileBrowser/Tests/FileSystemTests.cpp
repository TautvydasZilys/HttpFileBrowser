#include "PrecompiledHeader.h"

#if _TESTBUILD

#include "CppUnitTest.h"
#include "Utilities\StreamableFile.h"
#include "Utilities\Utilities.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;
using namespace Utilities;

namespace Microsoft
{
	namespace VisualStudio
	{
		namespace CppUnitTestFramework
		{
			template <>
			static std::wstring ToString<FileSystem::FileStatus>(const FileSystem::FileStatus& status)
			{
				switch (status)
				{
				case FileSystem::FileStatus::AccessDenied:
					return L"FileStatus::AccessDenied";

				case FileSystem::FileStatus::Directory:
					return L"FileStatus::Directory";

				case FileSystem::FileStatus::File:
					return L"FileStatus::File";

				case FileSystem::FileStatus::FileNotFound:
					return L"FileStatus::FileNotFound";
				}

				return L"Unknown file status";
			}
		}
	}
}

TEST_CLASS(FileSystemTests)
{
private:
	static vector<uint8_t> CreateRandomFile(uint64_t length, const wstring& path)
	{
		mt19937 randomEngine;
		uniform_int_distribution<int> distribution(numeric_limits<uint8_t>::min(), numeric_limits<uint8_t>::max());

		vector<uint8_t> bytes(static_cast<size_t>(length));

		for (auto& byte : bytes)
		{
			byte = static_cast<uint8_t>(distribution(randomEngine));
		}

		{
			ofstream out(path, ios::binary);
			out.write(reinterpret_cast<char*>(bytes.data()), bytes.size());
		}

		return bytes;
	}
	
public:
	TEST_METHOD(CanRemoveLastPathComponent)
	{
		const string expected = "C:\\Program Files\\Some Program\\";
		const string actual1 = FileSystem::RemoveLastPathComponent("C:\\Program Files\\Some Program\\Folder1 abc");
		const string actual2 = FileSystem::RemoveLastPathComponent("C:\\Program Files\\Some Program\\Folder1 abc\\");

		Assert::AreEqual(expected, actual1);
		Assert::AreEqual(expected, actual2);
	}

	TEST_METHOD(CanCombinePaths)
	{
		const string expected = "C:\\Program Files\\Some Program\\";
		const string actuals[] =
		{
			FileSystem::CombinePaths("C:\\Program Files\\", "Some Program\\"),
			FileSystem::CombinePaths("C:\\Program Files", "Some Program\\"),
			FileSystem::CombinePaths("C:\\Program Files\\Some Program\\", "."),
			FileSystem::CombinePaths("C:\\Program Files\\Some Program\\SomeFolder\\", ".."),
			FileSystem::CombinePaths("C:\\Program Files\\Some Program\\SomeFolder", "..")
		};

		for (auto& actual : actuals)
		{
			Assert::AreEqual(expected, actual);
		}
	}

	TEST_METHOD(CanFormatFileSizeString)
	{
		Assert::AreEqual("20 B", FileSystem::FormatFileSizeString(20).c_str());
		Assert::AreEqual("19.651 KB", FileSystem::FormatFileSizeString(20123).c_str());
		Assert::AreEqual("19.473 MB", FileSystem::FormatFileSizeString(20418912).c_str());
		Assert::AreEqual("2.287 GB", FileSystem::FormatFileSizeString(2456123456).c_str());
		Assert::AreEqual("7723.080 TB", FileSystem::FormatFileSizeString(8491616548941659).c_str());
	}

	TEST_METHOD(CanQueryFileStatus)
	{
		Assert::AreEqual(FileSystem::FileStatus::Directory, FileSystem::QueryFileStatus(L"C:\\Windows"));
		Assert::AreEqual(FileSystem::FileStatus::File, FileSystem::QueryFileStatus(L"C:\\Windows\\System32\\KernelBase.dll"));
		Assert::AreEqual(FileSystem::FileStatus::FileNotFound, FileSystem::QueryFileStatus(L"C:\\SomeRidiculousFileNameWithWhichFileWithCertainlyNotExist.really"));
	}

	TEST_METHOD(CanEnumerateFiles)
	{
		auto files = FileSystem::EnumerateFiles(L"C:\\Windows\\System32\\");
		auto kernelbaseDllExists = find_if(begin(files), end(files), [](const FileSystem::FileInfo& file)
		{
			return _stricmp(file.fileName.c_str(), "kernelbase.dll") == 0 && file.fileStatus == FileSystem::FileStatus::File;
		}) != end(files);

		auto driversFolderExists = find_if(begin(files), end(files), [](const FileSystem::FileInfo& file)
		{
			return _stricmp(file.fileName.c_str(), "drivers") == 0 && file.fileStatus == FileSystem::FileStatus::Directory;
		}) != end(files);

		Assert::IsTrue(kernelbaseDllExists);
		Assert::IsTrue(driversFolderExists);
	}

	TEST_METHOD(CanReadFileToVector)
	{
		const uint64_t kByteCount = 10456;
		const wstring kFileName = L"RandomFile.bin";

		auto bytes = CreateRandomFile(kByteCount, kFileName);
		auto fileBytes = FileSystem::ReadFileToVector(kFileName);

		Assert::AreEqual(kByteCount, static_cast<uint64_t>(fileBytes.size()));

		for (size_t i = 0; i < kByteCount; i++)
		{
			Assert::AreEqual(bytes[i], fileBytes[i]);
		}

		Assert::IsTrue(DeleteFileW(kFileName.c_str()) != FALSE);
	}

	TEST_METHOD(CanStreamFile)
	{
		const int kSizeOverChunkSize = 5;
		const uint64_t kByteCount = StreamableFile::kMaxChunkSize + kSizeOverChunkSize;
		const wstring kFileName = L"StreamableFile.bin";
		
		{
			unique_ptr<uint8_t[]> buffer(new uint8_t[StreamableFile::kMaxChunkSize]);
			auto bytes = CreateRandomFile(kByteCount, kFileName);
			StreamableFile streamableFile(kFileName);

			Assert::AreEqual(kByteCount, streamableFile.GetFileSize());
			Assert::IsFalse(streamableFile.IsEndOfFile());

			int bytesRead;
			size_t filePosition = 0;

			streamableFile.ReadNextChunk(reinterpret_cast<char*>(buffer.get()), bytesRead);
			Assert::AreEqual(StreamableFile::kMaxChunkSize, bytesRead);
			Assert::IsFalse(streamableFile.IsEndOfFile());
			Assert::IsTrue(memcmp(bytes.data() + filePosition, buffer.get(), StreamableFile::kMaxChunkSize) == 0);
			filePosition += StreamableFile::kMaxChunkSize;

			streamableFile.ReadNextChunk(reinterpret_cast<char*>(buffer.get()), bytesRead);
			Assert::AreEqual(kSizeOverChunkSize, bytesRead);
			Assert::IsTrue(streamableFile.IsEndOfFile());
			Assert::IsTrue(memcmp(bytes.data() + filePosition, buffer.get(), kSizeOverChunkSize) == 0);
		}

		Assert::IsTrue(DeleteFileW(kFileName.c_str()) != FALSE);
	}
};

#endif // _TESTBUILD
