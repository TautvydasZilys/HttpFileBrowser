#include "PrecompiledHeader.h"
#include "Initializer.h"

using namespace Utilities;

static void InitializeWinSock()
{
	WSAData wsaData;
	int wsaError = ERROR_SUCCESS;

	ZeroMemory(&wsaData, sizeof(wsaData));

	Logging::Log("Initializing WinSock 2.2.");

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

	Logging::LogFatalErrorIfFailed(wsaError != ERROR_SUCCESS, "Failed to initialize WinSock: ");
}

static void ShutdownWinSock()
{
	Logging::Log("Cleaning up WinSock.");

	auto cleanupResult = WSACleanup();
	Logging::LogErrorIfFailed(cleanupResult != NO_ERROR, "Failed to cleanup WinSock: ");
}

Initializer::Initializer()
{
	Logging::Initialize();
	InitializeWinSock();
}


Initializer::~Initializer()
{
	ShutdownWinSock();
	Logging::Shutdown();
}
