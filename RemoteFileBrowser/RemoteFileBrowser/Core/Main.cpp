#include "PrecompiledHeader.h"
#include "Communication\AssetDatabase.h"
#include "Communication\ClientServerConnection.h"
#include "Communication\FileBrowserResponseHandler.h"
#include "Http\Server.h"
#include "Tcp\Client.h"
#include "Tcp\Listener.h"
#include "Utilities\Event.h"
#include "Utilities\Initializer.h"

#if EXECUTABLE

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	Initializer winSockContext;
	AssetDatabase::Initialize();
	/*
	Tcp::Listener listener;
	listener.WhitelistIP(0);

	listener.RunAsync(INADDR_ANY, htons(18882), [](SOCKET incomingSocket, sockaddr_in clientAddress)
	{
		Http::Server::StartServiceClient(incomingSocket, clientAddress, &FileBrowserResponseHandler::ExecuteRequest);
	});

	// Wait indefinitely
	Event ev(false);
	ev.Wait();

	*/
	for (;;)
	{
		Tcp::Client::Connect(L"178.250.38.253", 1330, &ClientServerConnection::Create);
		Sleep(2000);	// If connection drops, wait 2 seconds and try again
	}

	return 0;
}

#endif