#include "PrecompiledHeader.h"
#include "Communication\AssetDatabase.h"
#include "Communication\ClientServerConnection.h"
#include "Communication\FileBrowserResponseHandler.h"
#include "Tcp\Client.h"
#include "Utilities\WSAInitializer.h"

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	WSAInitializer winSockContext;
	AssetDatabase::Initialize();

	for (;;)
	{
		Tcp::Client::Connect(L"localhost", 1335, &ClientServerConnection::Create);
		Sleep(2000);	// If connection drops, wait 2 seconds and try again
	}

	return 0;
}