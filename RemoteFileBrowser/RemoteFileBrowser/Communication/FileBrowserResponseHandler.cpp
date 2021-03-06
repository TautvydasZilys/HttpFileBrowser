#include "PrecompiledHeader.h"
#include "AssetDatabase.h"
#include "FileBrowserResponseHandler.h"
#include "SharedFiles.h"
#include "Utilities\StreamableFile.h"
#include "Utilities\Event.h"

using namespace std;
using namespace Utilities;

void FileBrowserResponseHandler::ExecuteRequest(SOCKET clientSocket, const string& requestedPath, const string& httpVersion)
{
	FileBrowserResponseHandler handler(clientSocket, requestedPath, httpVersion);
	handler.Execute();
}

FileBrowserResponseHandler::FileBrowserResponseHandler(SOCKET clientSocket, const string& requestedPath, const string& httpVersion) :
	m_ClientSocket(clientSocket),
	m_HttpVersion(httpVersion),
	m_RequestedPath(requestedPath), 
	m_FileStatus(FileSystem::QueryFileStatus(Encoding::Utf8ToUtf16(requestedPath))),
	m_ErrorCode(ERROR_SUCCESS)
{
	Logging::Log("Requested path: \"", requestedPath, "\".");
}

void FileBrowserResponseHandler::Execute()
{
	if (m_RequestedPath.length() > 1 && m_RequestedPath[1] != ':')
	{
		SendBuiltinFile();
		return;
	}

	if (m_FileStatus == FileSystem::FileStatus::File)
	{
		SendFileResponse();
	}
	else
	{
		SendHtmlResponse();
	}
}

bool FileBrowserResponseHandler::SendData(const char* data, size_t length) const
{
	Assert(length < static_cast<size_t>(numeric_limits<int>::max()));
	auto sendResult = send(m_ClientSocket, data, static_cast<int>(length), 0);

	if (sendResult == SOCKET_ERROR)
	{
		Logging::Error(WSAGetLastError(), "Failed to send response: ");
	}

	return sendResult == static_cast<int>(length);
}

void FileBrowserResponseHandler::SendNotFoundResponse() const
{
	auto httpHeader = m_HttpVersion + " 404 Not Found\r\n";
	SendData(httpHeader.c_str(), httpHeader.length());
}

void FileBrowserResponseHandler::SendFileResponse() const
{
	if (!SharedFiles::IsFileShared(m_RequestedPath))
	{
		SendNotFoundResponse();
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

		Logging::Error(GetLastError(), "Failed to send file \"", m_RequestedPath, "\": ");
		closesocket(m_ClientSocket);

		SetLastError(ERROR_SUCCESS);
	}
}

void FileBrowserResponseHandler::SendBuiltinFile() const
{
	stringstream httpResponse;
	string contentType;
	size_t contentLength;
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
		contentType = "text/css";
	}
	else
	{
		SendNotFoundResponse();
		return;
	}

	auto header = FormHttpHeaderForFile(contentType, m_RequestedPath, contentLength);
	SendData(header.c_str(), header.length());
	SendData(data, contentLength);
}

void FileBrowserResponseHandler::StreamFile() const
{
	StreamableFile file(Encoding::Utf8ToUtf16(m_RequestedPath));

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
	volatile bool doneSending = false;
	bool failed = false;

	Event dataReadyEvent(false),
		  bufferPtrReadEvent(false);

	thread sendingThread([this, &currentBuffer, &currentDataLength, &doneReading, &doneSending, &dataReadyEvent, &bufferPtrReadEvent]()
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

			if (!SendData(dataPtr, currentDataLength))
			{
				bufferPtrReadEvent.Set();
				doneSending = true;
				return;
			}
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
			
			const DWORD kTimeout = 30000;	// 30 seconds
			if (!bufferPtrReadEvent.Wait(kTimeout))	// Throw exception if the other thread fails to second data within 30 seconds
				throw exception();	// This usually happens when browser cancels download but doesn't close the socket

			if (doneSending)
				break;

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
				"<link rel=\"stylesheet\" type=\"text/css\" href=\"/style.css\" />"
				"<script src=\"/scripts.js\"></script>"
			"</head>";
}

void FileBrowserResponseHandler::FormHtmlResponseBody(stringstream& html) const
{
	auto upPath = Utilities::FileSystem::RemoveLastPathComponent(m_RequestedPath);
	Utilities::Encoding::EncodeUrlInline(upPath);

	html << "<body>"
				"<h1>HTTP File Browser</h1>"
				"<br/>"
				"<a href=/" << upPath << "><h2>Go up</h2></a>"
				"<h2>File system at path \"" << m_RequestedPath << "\":</h2>"
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
		case FileSystem::FileStatus::AccessDenied:
			GenerateHtmlBodyContentAccessDenied(html);
			break;

		case FileSystem::FileStatus::FileNotFound:
			GenerateHtmlBodyContentFileNotFound(html);
			break;

		case FileSystem::FileStatus::Directory:
			GenerateHtmlBodyContentOfDirectory(html);
			break;

		default:
			Logging::Log("ERROR: unexpected file status in FileBrowserResponseHandler::GenerateHtmlBodyContent (", to_string(static_cast<int>(m_FileStatus)), ").");
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

void FileBrowserResponseHandler::GenerateHtmlBodyContentOfDirectory(stringstream& html) const
{
	using namespace Utilities::FileSystem;

	if (m_RequestedPath.length() > MAX_PATH - 4)
	{
		GenerateHtmlBodyContentError(html, "Specified path is too long.");
		return;
	}

	if (!SharedFiles::IsFolderVisible(m_RequestedPath))
	{
		auto errorMessage = Encoding::Utf16ToUtf8(Logging::Win32ErrorToMessage(ERROR_PATH_NOT_FOUND));
		GenerateHtmlBodyContentError(html, errorMessage);
		return;
	}

	auto files = SharedFiles::GetFolderContents(m_RequestedPath);

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

			if (file.fileStatus == FileStatus::Directory)
			{
				fileType = "";
			}
			else
			{
				fileType = file.fileName.substr(file.fileName.find_last_of('.') + 1);
			}

			// Prepare file path for the hyperlink

			filePath = CombinePaths(m_RequestedPath, file.fileName);
			Encoding::EncodeUrlInline(filePath);
			
			// Format file size

			string fileSize;

			if (file.fileSize > 0)
			{
				fileSize = FormatFileSizeString(file.fileSize);
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
			auto wideErrorMessage = Logging::Win32ErrorToMessage(errorCode);
			auto errorMessage = Encoding::Utf16ToUtf8(wideErrorMessage);
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

	for (const auto& file : SharedFiles::GetVolumes())
	{
		html << "<tr><td><a href=\"/" << file << "\">" << file << "</a></td></tr>";
	}

	html << "</table>";
}