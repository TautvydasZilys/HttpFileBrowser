#include "PrecompiledHeader.h"
#include "AssetDatabase.h"
#include "FileBrowserResponseHandler.h"
#include "HttpServer.h"
#include "TcpListener.h"

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	AssetDatabase::Initialize();

	TcpListener::Initialize();
	TcpListener::Run(1337, [](SOCKET incomingSocket, sockaddr_in clientAddress)
	{
		HttpServer::StartServiceClient(incomingSocket, clientAddress, &FileBrowserResponseHandler::ExecuteRequest);
	});
	TcpListener::Cleanup();

	return 0;
}