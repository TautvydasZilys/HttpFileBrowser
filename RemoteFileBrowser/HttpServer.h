#pragma once

// 1st arg - relative request URL
// 2nd arg - http version
typedef std::function<std::string(const std::string&, const std::string&)> HttpRequestExecutionHandler;

class HttpServer
{
private:
	SOCKET m_ConnectionSocket;
	sockaddr_in m_ClientAddress;
	int m_BytesReceived;
	const char* m_ReceivedData;
	HttpRequestExecutionHandler m_ExecutionHandler;

private:
	HttpServer(SOCKET incomingSocket, sockaddr_in clientAddress, HttpRequestExecutionHandler executionHandler);
	void Run();

	void HandleRequest();
	std::string ParseRequest();
	std::string ExecuteRequest(const std::string& requestedPath, const std::string& httpVersion);
	std::string FormResponseHtml(const std::string& requestedPath);
	std::string FormFinalResponse(const std::string& html, const std::string& httpVersion);
	void SendResponse(const std::string& response);
	int FindNextCharacter(int position, char character);
	void ReportConnectionDroppedError();

public:
	static void StartServiceClient(SOCKET incomingSocket, sockaddr_in clientAddress, HttpRequestExecutionHandler executionHandler);
};