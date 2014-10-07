#pragma once

namespace RestCommunicator
{
	void Post(SOCKET s, const std::string& hostname, const std::string& path, const std::string& key, const std::string& value);
	bool ReceiveResponse(SOCKET s);

	bool ReceivePost(SOCKET s, std::unordered_map<std::string, std::string>& results);
};

