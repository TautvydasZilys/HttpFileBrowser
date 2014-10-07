#include "PrecompiledHeader.h"
#include "HttpHeaderBuilder.h"
#include "HttpRequest.h"
#include "RestCommunicator.h"

using namespace std;

static void SendPostRequest(SOCKET s, const string& hostname, const string& path, const string& httpBody)
{
	auto header = HttpHeaderBuilder::BuildPostHeader(hostname, path, "application/json", httpBody.length());

	auto result = send(s, header.c_str(), header.length(), 0);
	if (result != header.length()) goto fail;

	result = send(s, httpBody.c_str(), httpBody.length(), 0);
	if (result != httpBody.length()) goto fail;

	return;

fail:
	Assert(false);	
	Utilities::Logging::LogErrorIfFailed(true, L"Failed to send post request: ");
}

void RestCommunicator::Post(SOCKET s, const string& hostname, const string& path, const string& key, const string& value)
{
	// Body format:
	// {"<KEY>":"<VALUE>"}
	string httpBody;
	auto length = 0;
	httpBody.resize(7 + key.length() + value.length());

	httpBody[length++] = '{';
	httpBody[length++] = '\"';

	memcpy(&httpBody[length], key.data(), key.length());
	length += key.length();

	httpBody[length++] = '\"';
	httpBody[length++] = ':';
	httpBody[length++] = '\"';

	memcpy(&httpBody[length], value.data(), value.length());
	length += value.length();

	httpBody[length++] = '\"';
	httpBody[length++] = '}';

	SendPostRequest(s, hostname, path, httpBody);
}

bool RestCommunicator::ReceiveResponse(SOCKET s)
{
	const int kBufferLength = 1024;
	char buffer[kBufferLength];

	auto bytesReceived = recv(s, buffer, kBufferLength, 0);

	if (bytesReceived < 1)
	{
		return false;
	}

	auto bufferEnd = buffer + kBufferLength;
	auto newLine = std::find(buffer, bufferEnd, '\n');

	if (newLine == bufferEnd)
	{
		return false;
	}

	*newLine = '\0';
	return strcmp(buffer, "HTTP/1.1 200 OK") == 0;
}

bool RestCommunicator::ReceivePost(SOCKET s, unordered_map<string, string>& results)
{
	results.clear();

	const Http::Request receivedRequest(s);

	if (receivedRequest.requestVerb != Http::RequestVerb::POST || receivedRequest.contentType != Http::ContentType::JSON)
	{
		return false;
	}

	const auto& content = receivedRequest.content;

	// Make sure it really has a JSON body
	if (content.length() < 2 || content[0] != '{' ||
		content[content.length() - 1] != '}')
	{
		return false;
	}

	auto position = 1;
	while (position < content.length() - 1)
	{
		auto parenthesis = content.find('\"', position);
		if (parenthesis == string::npos) return false;

		auto closingParenthesis = content.find('\"', parenthesis + 1);
		if (closingParenthesis == string::npos) return false;

		auto key = content.substr(parenthesis + 1, closingParenthesis - parenthesis - 1);

		position = closingParenthesis + 1;
		if (position + 1 >= content.length() || content[position] != ':') return false;

		parenthesis = content.find('\"', position + 2);
		if (parenthesis == string::npos) return false;

		closingParenthesis = content.find('\"', parenthesis + 1);
		if (closingParenthesis == string::npos) return false;

		auto value = content.substr(parenthesis + 1, closingParenthesis - parenthesis - 1);

		results.emplace(std::move(key), std::move(value));
		position = closingParenthesis + 1;
	}
}