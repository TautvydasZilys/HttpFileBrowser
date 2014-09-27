#include "PrecompiledHeader.h"
#include "WSAInitializer.h"

using namespace Utilities;

WSAInitializer::WSAInitializer()
{
	WSAData wsaData;
	int wsaError = ERROR_SUCCESS;

	ZeroMemory(&wsaData, sizeof(wsaData));

	Logging::Log(L"Initializing WinSock 2.2.");

	do
	{
		auto wsaResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		Assert(wsaResult == NO_ERROR);

		if (wsaResult != 0)
		{
			wsaError = WSAGetLastError();
			Sleep(100);
		}
	} while (wsaError == WSASYSNOTREADY ||
		wsaError == WSAEINPROGRESS ||
		wsaError == WSAEPROCLIM);

	Logging::LogFatalErrorIfFailed(wsaError != ERROR_SUCCESS, L"Failed to initialize WinSock: ");
}


WSAInitializer::~WSAInitializer()
{
	Logging::Log(L"Cleaning up WinSock.");

	auto cleanupResult = WSACleanup();
	Logging::LogErrorIfFailed(cleanupResult != NO_ERROR, L"Failed to cleanup WinSock: ");
}
