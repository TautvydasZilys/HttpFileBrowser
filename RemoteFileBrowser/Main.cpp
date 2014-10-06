#include "PrecompiledHeader.h"
#include "AssetDatabase.h"
#include "ClientServerConnection.h"
#include "FileBrowserResponseHandler.h"
#include "HttpServer.h"
#include "TcpListener.h"
#include "TcpClient.h"
#include "WSAInitializer.h"

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	WSAInitializer winSockContext;
	AssetDatabase::Initialize();
	MessageBoxW(nullptr, L"Attach a debugger", L"", 0);
	for (;;)
	{
		TcpClient::Connect(L"localhost", 2891, &ClientServerConnection::Create);
		Sleep(2000);	// If connection drops, wait 2 seconds and try again
	}

	return 0;
}