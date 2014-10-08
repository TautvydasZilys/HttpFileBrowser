#pragma once

namespace RestCommunicator
{
	void Post(SOCKET s, std::string&& hostname, std::string&& path, const std::string& key, const std::string& value);
	bool ReceiveResponse(SOCKET s);

	bool ReceivePost(SOCKET s, std::unordered_map<std::string, std::string>& results);
};

