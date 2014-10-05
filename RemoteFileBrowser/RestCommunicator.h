#pragma once

namespace RestCommunicator
{
	void Post(SOCKET s, const std::string& path, const std::string& key, const std::string& value);

	bool Receive(SOCKET s, std::unordered_map<std::string, std::string>& results);
};

