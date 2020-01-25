#include "../tab-space/cefclient_win.h"

#include <chrono>
#include <thread>

#include "../cefclient/browser/root_window_manager.h"

#include "client_http.hpp"
#include "server_http.hpp"

#include "../tab-space/cef-info.h"
#include "../tab-space/webserver.h"

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
using HttpClient = SimpleWeb::Client<SimpleWeb::HTTP>;

int main() {
	// Windows setup.
	HINSTANCE hInstance = GetModuleHandle(NULL);
	HINSTANCE hPrevInstance = NULL;
	LPTSTR lpCmdLine = GetCommandLine();
	int nCmdShow = SW_SHOWNORMAL;

	//// Single-threaded webserver.
	//HttpServer httpServer;
	//setupHttpServer(httpServer);

	//// Run webserver in separate thread.
	//std::thread webserverThread([&]() {
	//	std::cout << "Webserver starting..." << std::endl;
	//	httpServer.start();
	//	std::cout << "Webserver terminated." << std::endl;

	//	return 0;
	//	}
	//);
	//webserverThread.detach();

	// Setup program state.
	CefInfo cefInfo;

	// Begin CEF loop in separate thread.
	client::RunMain::RunMain(hInstance, nCmdShow, cefInfo);

	//// Take commands in main thread.
	//std::this_thread::sleep_for(std::chrono::milliseconds(5000));
	//client::RootWindowConfig window_config;
	//window_config.always_on_top = false;
	//window_config.with_controls = true;
	//window_config.with_osr = false;
	//std::cout << "Launching root window...";
	//cefInfo.context->GetRootWindowManager()->CreateRootWindow(window_config);
	//webserverThread.join();
	return 0;
}
