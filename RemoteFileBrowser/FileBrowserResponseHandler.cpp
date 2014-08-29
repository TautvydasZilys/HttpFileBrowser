#include "PrecompiledHeader.h"
#include "FileBrowserResponseHandler.h"
#include "Utilities.h"

namespace FileBrowserResponseHandler
{
	using namespace std;

	static void FormResponseHeader(stringstream& html, const string& requestedPath)
	{
		html << "<head>"
					"<title>HTTP File Browser - " << requestedPath << "</title>"
					"<meta charset=\"utf-8\" />"
				"</head>";
	}

	static void FormResponseBody(stringstream& html, const string& requestedPath)
	{
		vector<string> results;

		if (requestedPath.empty())
		{
			results = Utilities::EnumerateSystemVolumes();
		}
		else
		{
			// Enumerate files
		}

		html << "<body>"
					"<h1>HTTP File Browser</h1>"
					"<br/>"
					"<h2>File system at path \"" << requestedPath << "\":"
					"<table>";

		for (const auto& file : results)
		{
			html << "<tr><td><a href=\"/" << file << "\">" << file << "</a></td></tr>";
		}

		html <<		"</table>"
				"</body>";
	}

	static string FormResponseHtml(const string& requestedPath)
	{
		stringstream html;

		html << "<!DOCTYPE html>"
				"<html>";

		FormResponseHeader(html, requestedPath);
		FormResponseBody(html, requestedPath);

		html << "</html>";

		return html.str();
	}

	static string FormFinalResponse(const string& html, const string& httpVersion)
	{
		stringstream httpResponse;

		httpResponse << httpVersion << " 200 OK\r\n";
		httpResponse << "Content-Type: text/html; charset=utf-8\r\n";
		httpResponse << "Content-Length: " << html.length() << "\r\n\r\n";
		httpResponse << html;

		return httpResponse.str();
	}

	string ExecuteRequest(const string& requestedPath, const string& httpVersion)
	{
		auto html = FormResponseHtml(requestedPath);
		return FormFinalResponse(html, httpVersion);
	}
}