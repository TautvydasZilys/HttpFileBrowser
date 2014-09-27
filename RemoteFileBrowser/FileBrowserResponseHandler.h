#pragma once

namespace Utilities
{
	namespace FileSystem
	{
		enum class FileStatus;
	}
}

class FileBrowserResponseHandler
{
private:
	SOCKET m_ClientSocket;
	const std::string& m_HttpVersion;
	const std::string& m_RequestedPath;
	std::wstring m_WidePath;
	Utilities::FileSystem::FileStatus m_FileStatus;
	int m_ErrorCode;

private:
	FileBrowserResponseHandler(SOCKET clientSocket, const std::string& requestedPath, const std::string& httpVersion);
	void Execute();

	void SendData(const char* data, int length) const;
	void SendNotFoundResponse() const;

	void SendFileResponse() const;
	void SendBuiltinFile() const;
	void StreamFile() const;

	std::string FormHttpHeaderForFile(const std::string& contentType, const std::string& fileName, uint64_t fileLength) const;

	void SendHtmlResponse() const;
	std::string FormHtmlResponse() const;
	std::string WrapHtmlInHttpHeader(const std::string& html) const;

	void FormHtmlResponseHead(std::stringstream& html) const;
	void FormHtmlResponseBody(std::stringstream& html) const;
	void GenerateHtmlBodyContent(std::stringstream& html) const;

	void GenerateHtmlBodyContentAccessDenied(std::stringstream& html) const;
	void GenerateHtmlBodyContentError(std::stringstream& html, const std::string& errorMessage) const;
	void GenerateHtmlBodyContentFileNotFound(std::stringstream& html) const;
	void GenerateHtmlBodyContentOfDirectory(std::stringstream& html) const;
	void GenerateHtmlBodyContentOfSystemVolumes(std::stringstream& html) const;

public:
	static void ExecuteRequest(SOCKET clientSocket, const std::string& requestedPath, const std::string& httpVersion);
};