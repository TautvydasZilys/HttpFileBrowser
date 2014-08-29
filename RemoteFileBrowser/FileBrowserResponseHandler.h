#pragma once

namespace Utilities
{
	enum class FileStatus;
}

class FileBrowserResponseHandler
{
private:
	const std::string& m_HttpVersion;
	const std::string& m_RequestedPath;
	std::wstring m_WidePath;
	Utilities::FileStatus m_FileStatus;
	int m_ErrorCode;

private:
	FileBrowserResponseHandler(const std::string& requestedPath, const std::string& httpVersion);
	std::string Execute();

	std::string WrapHtmlInHttpHeader(const std::string& html);

	std::string FormFileResponse();	
	std::string FormHtmlResponse();

	void FormHtmlResponseHead(std::stringstream& html);
	void FormHtmlResponseBody(std::stringstream& html);
	void GenerateHtmlBodyContent(std::stringstream& html);

	void GenerateHtmlBodyContentAccessDenied(std::stringstream& html);
	void GenerateHtmlBodyContentError(std::stringstream& html, const std::string& errorMessage);
	void GenerateHtmlBodyContentFileNotFound(std::stringstream& html);
	void GenerateHtmlBodyContentFileDownloadError(std::stringstream& html);
	void GenerateHtmlBodyContentOfDirectory(std::stringstream& html);
	void GenerateHtmlBodyContentOfSystemVolumes(std::stringstream& html);

public:
	static std::string ExecuteRequest(const std::string& requestedPath, const std::string& httpVersion);
};