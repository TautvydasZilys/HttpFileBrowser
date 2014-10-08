#include "PrecompiledHeader.h"
#include "AssetDatabase.h"

namespace AssetDatabase
{
	using namespace std;
	using namespace Utilities;

	static vector<uint8_t> s_ScriptsFile;
	static vector<uint8_t> s_StyleFile;

	void Initialize()
	{
		s_ScriptsFile = FileSystem::ReadFileToVector(L"scripts.js");
		s_StyleFile = FileSystem::ReadFileToVector(L"style.css");
	}

	const std::vector<uint8_t>& GetScriptsFile()
	{
		return s_ScriptsFile;
	}

	const std::vector<uint8_t>& GetStyleFile()
	{
		return s_StyleFile;
	}
}