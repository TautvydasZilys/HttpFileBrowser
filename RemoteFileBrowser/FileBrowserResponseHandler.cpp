#include "PrecompiledHeader.h"
#include "FileBrowserResponseHandler.h"
#include "Utilities.h"

using namespace std;

string FileBrowserResponseHandler::ExecuteRequest(const string& requestedPath, const string& httpVersion)
{
	FileBrowserResponseHandler handler(requestedPath, httpVersion);
	return handler.Execute();
}

FileBrowserResponseHandler::FileBrowserResponseHandler(const string& requestedPath, const string& httpVersion) :
	m_HttpVersion(httpVersion), 
	m_RequestedPath(requestedPath), 
	m_WidePath(Utilities::Utf8ToUtf16(requestedPath)),
	m_FileStatus(Utilities::QueryFileStatus(m_WidePath)),
	m_ErrorCode(ERROR_SUCCESS)
{
}

string FileBrowserResponseHandler::Execute()
{
	if (m_FileStatus == Utilities::FileStatus::File)
	{
		return FormFileResponse();
	}

	return FormHtmlResponse();
}

string FileBrowserResponseHandler::WrapHtmlInHttpHeader(const string& html)
{
	stringstream httpResponse;

	httpResponse << m_HttpVersion << " 200 OK\r\n";
	httpResponse << "Content-Type: text/html; charset=utf-8\r\n";
	httpResponse << "Content-Length: " << html.length() << "\r\n\r\n";
	httpResponse << html;

	return httpResponse.str();
}

string FileBrowserResponseHandler::FormFileResponse()
{
	stringstream httpResponse;

	auto fileName = m_RequestedPath.substr(m_RequestedPath.find_last_of('\\') + 1);

	httpResponse << m_HttpVersion << " 200 OK\r\n";
	httpResponse << "Content-Type: application/force-download\r\n";
	httpResponse << "Content-Disposition: attachment; filename=\"" << fileName << "\"\r\n";
	httpResponse << "Content-Length: ";

	auto httpHeader = httpResponse.str();

	if (Utilities::AppendFileLengthAndReadItWholeTo(m_WidePath, httpHeader))
	{
		Utilities::Log(L"Sending back \"" + m_WidePath + L"\".");
		return httpHeader;
	}

	// Handle failure

	m_ErrorCode = GetLastError();
	SetLastError(ERROR_SUCCESS);
	return FormHtmlResponse();
}

string FileBrowserResponseHandler::FormHtmlResponse()
{
	stringstream html;

	html << "<!DOCTYPE html>"
			"<html>";

	FormHtmlResponseHead(html);
	FormHtmlResponseBody(html);

	html << "</html>";

	return WrapHtmlInHttpHeader(html.str());
}

void FileBrowserResponseHandler::FormHtmlResponseHead(stringstream& html)
{
	html << "<head>"
				"<title>HTTP File Browser - " << m_RequestedPath << "</title>"
				"<meta charset=\"utf-8\" />"
			"</head>";
}

void FileBrowserResponseHandler::FormHtmlResponseBody(stringstream& html)
{
	html << "<body>"
				"<h1>HTTP File Browser</h1>"
				"<br/>"
				"<h2>File system at path \"" << m_RequestedPath << "\":"
				"<br/>"
				"<br/>";

	GenerateHtmlBodyContent(html);

	html << "</body>";
}

void FileBrowserResponseHandler::GenerateHtmlBodyContent(stringstream& html)
{
	if (m_RequestedPath.empty())
	{
		GenerateHtmlBodyContentOfSystemVolumes(html);
	}
	else
	{
		switch (m_FileStatus)
		{
		case Utilities::FileStatus::AccessDenied:
			GenerateHtmlBodyContentAccessDenied(html);
			break;

		case Utilities::FileStatus::FileNotFound:
			GenerateHtmlBodyContentFileNotFound(html);
			break;

		case Utilities::FileStatus::Directory:
			GenerateHtmlBodyContentOfDirectory(html);
			break;

		case Utilities::FileStatus::File:	// This will only be hit if error was triggered while trying to download the file
			GenerateHtmlBodyContentFileDownloadError(html);
			break;

		default:
			Utilities::Log(L"ERROR: unexpected file status in FileBrowserResponseHandler::GenerateHtmlBodyContent (" + to_wstring(static_cast<int>(m_FileStatus)) + L").");
		}
	}
}

void FileBrowserResponseHandler::GenerateHtmlBodyContentError(std::stringstream& html, const string& errorMessage)
{
	html << "<font color=\"red\">" << errorMessage << "</font>";
	html << "<br/><br/>";
	html << "<a href=\"/\">Return to homepage</a>";
}

void FileBrowserResponseHandler::GenerateHtmlBodyContentAccessDenied(stringstream& html)
{
	auto errorMessage = "Error: access to \"" + m_RequestedPath + "\" is denied.";
	GenerateHtmlBodyContentError(html, errorMessage);
}

void FileBrowserResponseHandler::GenerateHtmlBodyContentFileNotFound(stringstream& html)
{
	auto errorMessage = "Error: \"" + m_RequestedPath + "\" does not exist.";
	GenerateHtmlBodyContentError(html, errorMessage);
}

void FileBrowserResponseHandler::GenerateHtmlBodyContentFileDownloadError(stringstream& html)
{
	auto wideErrorMessage = Utilities::Win32ErrorToMessage(m_ErrorCode);
	auto errorMessage = "Error - failed to download file: " + Utilities::Utf16ToUtf8(wideErrorMessage);

	GenerateHtmlBodyContentError(html, errorMessage);
}

void FileBrowserResponseHandler::GenerateHtmlBodyContentOfDirectory(stringstream& html)
{
	if (m_WidePath.length() > MAX_PATH - 4)
	{
		GenerateHtmlBodyContentError(html, "Specified path is too long.");
		return;
	}

	auto files = Utilities::EnumerateFiles(m_WidePath);

	if (files.size() > 0)
	{
		html << "<table>";

		html << "<tr>"
					"<th>File name</th>"
					"<th>File type</th>"
					"<th>Date modified</th>"
				"</tr>";

		for (auto& file : files)
		{
			string filePath;
			string fileType;

			if (file.fileStatus == Utilities::FileStatus::Directory)
			{
				fileType = "Directory";
			}
			else
			{
				fileType = file.fileName.substr(file.fileName.find_last_of('.') + 1);
			}

			if (m_RequestedPath[m_RequestedPath.length() - 1] == '\\')
			{
				filePath = Utilities::EncodeUrl(m_RequestedPath + file.fileName);
			}
			else
			{
				filePath = Utilities::EncodeUrl(m_RequestedPath + '\\' + file.fileName);
			}
			
			html << "<tr>"
						"<td><a href=\"/" << filePath << "\">" << file.fileName << "</a></td>"
						"<td>" << fileType << "</td>"
						"<td>" << file.dateModified << "</td>"
					"</tr>";
		}

		html << "</table>";
	}
	else
	{
		auto errorCode = GetLastError();

		if (errorCode != ERROR_SUCCESS)
		{
			auto wideErrorMessage = Utilities::Win32ErrorToMessage(errorCode);
			auto errorMessage = Utilities::Utf16ToUtf8(wideErrorMessage);
			GenerateHtmlBodyContentError(html, errorMessage);
		}
		else
		{
			html << "The directory is empty.";
		}
	}
}

void FileBrowserResponseHandler::GenerateHtmlBodyContentOfSystemVolumes(stringstream& html)
{
	html << "<table>";

	for (const auto& file : Utilities::EnumerateSystemVolumes())
	{
		html << "<tr><td><a href=\"/" << file << "\">" << file << "</a></td></tr>";
	}

	html << "</table>";
}