#include "PrecompiledHeader.h"
#include "Request.h"
#include "RestCommunicator.h"

using namespace std;

static const char kOkResponse[] = "HTTP/1.1 200 OK";
static const char kBadRequestResponse[] = "HTTP/1.1 400 Bad Request";

static void SendPostRequest(SOCKET s, const Http::Request& httpRequest)
{
	auto header = httpRequest.BuildHeaderString();

	Assert(header.length() < numeric_limits<int>::max());
	auto result = send(s, header.c_str(), static_cast<int>(header.length()), 0);
	if (result != header.length()) goto fail;

	Assert(httpRequest.content.length() < numeric_limits<int>::max());
	result = send(s, httpRequest.content.c_str(), static_cast<int>(httpRequest.content.length()), 0);
	if (result != httpRequest.content.length()) goto fail;

	return;

fail:
	Assert(false);	
	Utilities::Logging::LogErrorIfFailed(true, "Failed to send post request: ");
}

void Http::RestCommunicator::Post(SOCKET s, string&& hostname, string&& path, const string& key, const string& value)
{
	Http::Request request;

	request.requestVerb = Http::RequestVerb::POST;
	request.contentType = Http::ContentType::JSON;
	request.hostname = std::move(hostname);
	request.requestPath = std::move(path);

	// Body format:
	// {"<KEY>":"<VALUE>"}
	size_t length = 0;
	request.content.resize(7 + key.length() + value.length());

	request.content[length++] = '{';
	request.content[length++] = '\"';

	memcpy(&request.content[length], key.data(), key.length());
	length += key.length();

	request.content[length++] = '\"';
	request.content[length++] = ':';
	request.content[length++] = '\"';

	memcpy(&request.content[length], value.data(), value.length());
	length += value.length();

	request.content[length++] = '\"';
	request.content[length++] = '}';
	
	SendPostRequest(s, request);
}

bool Http::RestCommunicator::ReceiveResponse(SOCKET s)
{
	const int kBufferLength = 1024;
	char buffer[kBufferLength];

	auto bytesReceived = recv(s, buffer, kBufferLength, 0);

	if (bytesReceived < 1)
	{
		return false;
	}

	return strncmp(buffer, kOkResponse, sizeof(kOkResponse) - 1) == 0;	// Don't compare null terminator
}

bool Http::RestCommunicator::ReceivePost(SOCKET s, unordered_map<string, string>& results)
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

	size_t position = 1;
	while (position < content.length() - 1)
	{
		auto quote = content.find('\"', position);
		if (quote == string::npos) return false;

		auto closingQuote = content.find('\"', quote + 1);
		if (closingQuote == string::npos) return false;

		auto key = content.substr(quote + 1, closingQuote - quote - 1);

		position = closingQuote + 1;
		if (position + 1 >= content.length() || content[position] != ':') return false;

		quote = content.find('\"', position + 1);
		if (quote == string::npos) return false;

		closingQuote = content.find('\"', quote + 1);
		if (closingQuote == string::npos) return false;

		auto value = content.substr(quote + 1, closingQuote - quote - 1);

		results.emplace(std::move(key), std::move(value));
		position = closingQuote + 1;
	}

	return true;
}

void Http::RestCommunicator::SendResponse(SOCKET s, bool success)
{
	if (success)
	{
		send(s, kOkResponse, sizeof(kOkResponse) - 1, 0);
	}
	else
	{
		send(s, kBadRequestResponse, sizeof(kBadRequestResponse) - 1, 0);
	}
}