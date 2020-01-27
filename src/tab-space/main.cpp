#include <chrono>
#include <thread>
#include <malloc.h>

#include "client_http.hpp"
#include "server_http.hpp"

#include "../cefclient/browser/root_window_manager.h"

#include "../rain-library-4/gdi-plus-include.h"

#include "../tab-space/cefclient_win.h"
#include "../tab-space/tab-space-state.h"
#include "../tab-space/webserver.h"

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
using HttpClient = SimpleWeb::Client<SimpleWeb::HTTP>;

int GetEncoderClsid(const WCHAR *format, CLSID *pClsid) {
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	Gdiplus::ImageCodecInfo *pImageCodecInfo = NULL;

	Gdiplus::GetImageEncodersSize(&num, &size);
	if (size == 0)
		return -1;  // Failure

	pImageCodecInfo = (Gdiplus::ImageCodecInfo *)(malloc(size));
	if (pImageCodecInfo == NULL)
		return -1;  // Failure

	GetImageEncoders(num, size, pImageCodecInfo);

	for (UINT j = 0; j < num; ++j)
	{
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success
		}
	}

	free(pImageCodecInfo);
	return -1;  // Failure
}

int webserverStart(TabSpaceState &tabSpaceState) {
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	// tabSpaceState.rng.seed(std::chrono::system_clock::now().time_since_epoch().count());

	GetEncoderClsid(L"image/jpeg", &tabSpaceState.jpegClsid);

	HttpServer httpServer;
	setupHttpServer(httpServer, tabSpaceState);
	std::cout << "Webserver starting on thread " << std::this_thread::get_id() << "..." << std::endl;
	httpServer.start();
	// std::cout << "Webserver terminated." << std::endl;


	Gdiplus::GdiplusShutdown(gdiplusToken);

	return 0;
}

int mainLogicStart(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow, TabSpaceState &tabSpaceState) {
	// std::cout << "Starting main logic thread..." << std::endl;

	// TODO: Why does starting the webserver thread here not work?
	/*std::this_thread::sleep_for(std::chrono::milliseconds(20000));
	client::RootWindowConfig window_config;
	window_config.always_on_top = false;
	window_config.with_controls = false;
	window_config.with_osr = false;
	std::cout << "Launching root window..." << std::endl;
	tabSpaceState.context->GetRootWindowManager()->CreateRootWindow(window_config);*/

	/*std::thread heartbeatThread([&]() {
		while (true) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			std::cout << "ba-dump" << std::endl;
		}
		}
	);*/

	// TODO: Take commands in main thread.
	while (true) {
		std::cin.get();
	}

	// std::cout << "Terminating main logic thread..." << std::endl;
	return 0;
}

int main() {
	// Windows setup.
	HINSTANCE hInstance = GetModuleHandle(NULL);
	HINSTANCE hPrevInstance = NULL;
	LPTSTR lpCmdLine = GetCommandLine();
	int nCmdShow = SW_SHOWNORMAL;

	// std::cout << "Starting process..." << std::endl;

	// This process will be started multiple times. Ensure that in each, the CEF loop is run on the main thread. However, other threads should only be started from the main process.

	// Setup CEF state.
	TabSpaceState tabSpaceState;
	tabSpaceState.mainLogicFunction = mainLogicStart;
	tabSpaceState.webserverFunction = webserverStart;
	return client::RunMain::RunMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow, tabSpaceState);
}
