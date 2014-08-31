#include "PrecompiledHeader.h"
#include "AssetDatabase.h"
#include "Event.h"
#include "FileBrowserResponseHandler.h"
#include "StreamableFile.h"
#include "Utilities.h"

using namespace std;

void FileBrowserResponseHandler::ExecuteRequest(SOCKET clientSocket, const string& requestedPath, const string& httpVersion)
{
	FileBrowserResponseHandler handler(clientSocket, requestedPath, httpVersion);
	handler.Execute();
}

FileBrowserResponseHandler::FileBrowserResponseHandler(SOCKET clientSocket, const string& requestedPath, const string& httpVersion) :
	m_ClientSocket(clientSocket),
	m_HttpVersion(httpVersion), 
	m_RequestedPath(requestedPath), 
	m_WidePath(Utilities::Utf8ToUtf16(requestedPath)),
	m_FileStatus(Utilities::QueryFileStatus(m_WidePath)),
	m_ErrorCode(ERROR_SUCCESS)
{
	Utilities::Log(L"Requested path: \"" + m_WidePath + L"\".");
}

void FileBrowserResponseHandler::Execute()
{
	if (m_FileStatus == Utilities::FileStatus::File)
	{
		SendFileResponse();
	}
	else
	{
		SendHtmlResponse();
	}
}

void FileBrowserResponseHandler::SendData(const char* data, int length) const
{
	auto sendResult = send(m_ClientSocket, data, length, 0);
	Assert(sendResult != SOCKET_ERROR);

	if (sendResult == SOCKET_ERROR)
	{
		Utilities::Error(WSAGetLastError(), L"Failed to send response: ");
	}
}

void FileBrowserResponseHandler::SendFileResponse() const
{
	if (m_RequestedPath == "scripts.js" || m_RequestedPath == "style.css")
	{
		SendBuiltinFile();
		return;
	}

	try
	{
		StreamFile();
		return;
	}
	catch (exception)
	{
		// StreamFile will throw exception on failure

		Utilities::Error(GetLastError(), L"Failed to send file \"" + m_WidePath + L"\": ");
		closesocket(m_ClientSocket);

		SetLastError(ERROR_SUCCESS);
	}
}

void FileBrowserResponseHandler::SendBuiltinFile() const
{
	stringstream httpResponse;
	string contentType;
	int contentLength;
	const char* data;

	if (m_RequestedPath == "scripts.js")
	{
		auto& file = AssetDatabase::GetScriptsFile();
		contentLength = file.size();
		data = reinterpret_cast<const char*>(file.data());
		contentType = "application/javascript";
	}
	else if (m_RequestedPath == "style.css")
	{
		auto& file = AssetDatabase::GetStyleFile();
		contentLength = file.size();
		data = reinterpret_cast<const char*>(file.data());
		contentType == "text/css";
	}
	else
	{
		Utilities::Log(L"Unexpected file in FileBrowserResponseHandler::SendBuiltinFile(): " + m_WidePath);
		return;
	}

	auto header = FormHttpHeaderForFile(contentType, m_RequestedPath, contentLength);
	SendData(header.c_str(), header.length());
	SendData(data, contentLength);
}

void FileBrowserResponseHandler::StreamFile() const
{
	StreamableFile file(m_WidePath);

	// Form and send the header

	auto fileName = m_RequestedPath.substr(m_RequestedPath.find_last_of('\\') + 1);
	auto httpHeader = FormHttpHeaderForFile("application/force-download", fileName, file.GetFileSize());

	SendData(httpHeader.c_str(), httpHeader.length());

	// Stream the file

	int bytesRead = 0;
	const int kBufferCount = 2;
	unique_ptr<char[]> buffers[kBufferCount];

	for (auto& buffer : buffers)
	{
		buffer = unique_ptr<char[]>(new char[StreamableFile::kMaxChunkSize]);
	}

	int currentBufferIndex = 1;
	char* volatile currentBuffer;
	volatile int currentDataLength;
	volatile bool doneReading = false;
	bool failed = false;

	Event dataReadyEvent(false),
		  bufferPtrReadEvent(false);

	thread sendingThread([this, &currentBuffer, &currentDataLength, &doneReading, &dataReadyEvent, &bufferPtrReadEvent]()
	{
		for (;;)
		{
			dataReadyEvent.Wait();

			if (doneReading)
			{
				return;
			}

			auto dataPtr = currentBuffer;
			auto length = currentDataLength;

			bufferPtrReadEvent.Set();

			SendData(dataPtr, currentDataLength);
		}
	});

	while (!file.IsEndOfFile())
	{
		try
		{
			file.ReadNextChunk(buffers[currentBufferIndex].get(), bytesRead);

			currentBuffer = buffers[currentBufferIndex].get();
			currentDataLength = bytesRead;

			dataReadyEvent.Set();
			bufferPtrReadEvent.Wait();

			currentBufferIndex = (currentBufferIndex + 1) % kBufferCount;
		}
		catch (exception)
		{
			doneReading = true;
			dataReadyEvent.Set();
			sendingThread.join();
			throw;
		}
	}

	doneReading = true;
	dataReadyEvent.Set();
	sendingThread.join();
}

string FileBrowserResponseHandler::FormHttpHeaderForFile(const string& contentType, const string& fileName, uint64_t fileSize) const
{
	stringstream httpHeader;

	httpHeader << m_HttpVersion << " 200 OK\r\n";
	httpHeader << "Content-Type: " << contentType << "\r\n";
	httpHeader << "Content-Disposition: attachment; filename=\"" << fileName << "\"\r\n";
	httpHeader << "Content-Length: " << fileSize << "\r\n\r\n";

	return httpHeader.str();
}

void FileBrowserResponseHandler::SendHtmlResponse() const
{
	auto response = FormHtmlResponse();
	SendData(response.data(), static_cast<int>(response.length()));
}

string FileBrowserResponseHandler::FormHtmlResponse() const
{
	stringstream html;

	html << "<!DOCTYPE html>"
			"<html>";

	FormHtmlResponseHead(html);
	FormHtmlResponseBody(html);

	html << "</html>";

	return WrapHtmlInHttpHeader(html.str());
}

string FileBrowserResponseHandler::WrapHtmlInHttpHeader(const string& html) const
{
	stringstream httpResponse;

	httpResponse << m_HttpVersion << " 200 OK\r\n";
	httpResponse << "Content-Type: text/html; charset=utf-8\r\n";
	httpResponse << "Content-Length: " << html.length() << "\r\n\r\n";
	httpResponse << html;

	return httpResponse.str();
}

void FileBrowserResponseHandler::FormHtmlResponseHead(stringstream& html) const
{
	html << "<head>"
				"<title>HTTP File Browser - " << m_RequestedPath << "</title>"
				"<meta charset=\"utf-8\" />"
				"<link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\" />"
				"<script src=\"scripts.js\"></script>"
			"</head>";
}

void FileBrowserResponseHandler::FormHtmlResponseBody(stringstream& html) const
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

void FileBrowserResponseHandler::GenerateHtmlBodyContent(stringstream& html) const
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

void FileBrowserResponseHandler::GenerateHtmlBodyContentError(stringstream& html, const string& errorMessage) const
{
	html << "<font color=\"red\">" << errorMessage << "</font>";
	html << "<br/><br/>";
	html << "<a href=\"/\">Return to homepage</a>";
}

void FileBrowserResponseHandler::GenerateHtmlBodyContentAccessDenied(stringstream& html) const
{
	auto errorMessage = "Error: access to \"" + m_RequestedPath + "\" is denied.";
	GenerateHtmlBodyContentError(html, errorMessage);
}

void FileBrowserResponseHandler::GenerateHtmlBodyContentFileNotFound(stringstream& html) const
{
	auto errorMessage = "Error: \"" + m_RequestedPath + "\" does not exist.";
	GenerateHtmlBodyContentError(html, errorMessage);
}

void FileBrowserResponseHandler::GenerateHtmlBodyContentFileDownloadError(stringstream& html) const
{
	auto wideErrorMessage = Utilities::Win32ErrorToMessage(m_ErrorCode);
	auto errorMessage = "Error - failed to download file: " + Utilities::Utf16ToUtf8(wideErrorMessage);

	GenerateHtmlBodyContentError(html, errorMessage);
}

void FileBrowserResponseHandler::GenerateHtmlBodyContentOfDirectory(stringstream& html) const
{
	if (m_WidePath.length() > MAX_PATH - 4)
	{
		GenerateHtmlBodyContentError(html, "Specified path is too long.");
		return;
	}

	auto files = Utilities::EnumerateFiles(m_WidePath);

	if (files.size() > 0)
	{
		html << "<table class=sortable>";

		html << "<tr>"
					"<th>File name</th>"
					"<th>File type</th>"
					"<th>File size</th>"
					"<th>Date modified</th>"
				"</tr>";

		for (auto& file : files)
		{
			string filePath;
			string fileType;

			// Figure out file type

			if (file.fileStatus == Utilities::FileStatus::Directory)
			{
				fileType = "";
			}
			else
			{
				fileType = file.fileName.substr(file.fileName.find_last_of('.') + 1);
			}

			// Prepare file path for the hyperlink

			filePath = Utilities::CombinePaths(m_RequestedPath, file.fileName);
			Utilities::EncodeUrlInline(filePath);
			
			// Format file size

			string fileSize;

			if (file.fileSize > 0)
			{
				fileSize = Utilities::FormatFileSize(file.fileSize);
			}

			html << "<tr>"
						"<td><a href=\"/" << filePath << "\">" << file.fileName << "</a></td>"
						"<td>" << fileType << "</td>"
						"<td>" << fileSize << "</td>"
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

void FileBrowserResponseHandler::GenerateHtmlBodyContentOfSystemVolumes(stringstream& html) const
{
	html << "<table>";

	for (const auto& file : Utilities::EnumerateSystemVolumes())
	{
		html << "<tr><td><a href=\"/" << file << "\">" << file << "</a></td></tr>";
	}

	html << "</table>";
}