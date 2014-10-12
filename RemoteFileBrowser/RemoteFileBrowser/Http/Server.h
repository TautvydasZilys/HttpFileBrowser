#pragma once

namespace Http
{
	// 1st arg - client socket
	// 2nd arg - relative request URL
	// 3rd arg - http version
	typedef std::function<void(SOCKET, const std::string&, const std::string&)> HttpRequestExecutionHandler;

	class Server
	{
	private:
		SOCKET m_ConnectionSocket;
		sockaddr_in6 m_ClientAddress;
		int m_BytesReceived;
		const char* m_ReceivedData;
		bool m_HasReportedUserAgent;
		HttpRequestExecutionHandler m_ExecutionHandler;

	private:
		Server(SOCKET incomingSocket, sockaddr_in6 clientAddress, HttpRequestExecutionHandler executionHandler);
		void Run();

		void HandleRequest();
		std::string ParseRequest();
		std::string ExecuteRequest(const std::string& requestedPath, const std::string& httpVersion);
		std::string FormResponseHtml(const std::string& requestedPath);
		std::string FormFinalResponse(const std::string& html, const std::string& httpVersion);
		void SendResponse(const std::string& response);
		int FindNextCharacter(int position, char character);
		void ReportUserAgent(int dataOffset);
		void ReportConnectionDroppedError();

	public:
		static void StartServiceClient(SOCKET incomingSocket, sockaddr_in6 clientAddress, HttpRequestExecutionHandler executionHandler);
	};
}