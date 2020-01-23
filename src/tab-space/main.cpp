#include "../tab-space/cefclient_win.h"

#include "client_http.hpp"
#include "server_http.hpp"

#include "../tab-space/webserver.h"

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
using HttpClient = SimpleWeb::Client<SimpleWeb::HTTP>;

int main() {
	// Windows setup.
	HINSTANCE hInstance = GetModuleHandle(NULL);
	HINSTANCE hPrevInstance = NULL;
	LPTSTR lpCmdLine = GetCommandLine();
	int nCmdShow = SW_SHOWNORMAL;

	// Single-threaded webserver.
	HttpServer server;
	setupHttpServer(server);
	std::thread serverThread([&server]() {
		server.start();
		}
	);
	serverThread.detach();

	// Blocks until CEF returns.
	int cefReturn = client::RunMain::RunMain(hInstance, nCmdShow);
	std::cout << "CEF terminated." << std::endl;
	serverThread.join();
	std::cout << "Webserver terminated." << std::endl;
	return cefReturn;
}
