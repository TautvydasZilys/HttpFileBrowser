#pragma once

namespace AssetDatabase
{
	void Initialize();

	const std::vector<uint8_t>& GetScriptsFile();
	const std::vector<uint8_t>& GetStyleFile();
};

