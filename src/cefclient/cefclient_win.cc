// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include <windows.h>
#include <iostream>

#include "include/base/cef_scoped_ptr.h"
#include "include/cef_command_line.h"
#include "include/cef_sandbox_win.h"
#include "../cefclient/browser/main_context_impl.h"
#include "../cefclient/browser/main_message_loop_multithreaded_win.h"
#include "../cefclient/browser/root_window_manager.h"
#include "../cefclient/browser/test_runner.h"
#include "../shared/browser/client_app_browser.h"
#include "../shared/browser/main_message_loop_external_pump.h"
#include "../shared/browser/main_message_loop_std.h"
#include "../shared/common/client_app_other.h"
#include "../shared/common/client_switches.h"
#include "../shared/renderer/client_app_renderer.h"

// #include "../tab-space/rain-window.h"

#include "../tab-space/tab-space-state.h"

// tab-space: Sandbox is manually turned off; i.e. CEF_USE_SANDBOX is not defined.

// When generating projects with CMake the CEF_USE_SANDBOX value will be defined
// automatically if using the required compiler version. Pass -DUSE_SANDBOX=OFF
// to the CMake command-line to disable use of the sandbox.
// Uncomment this line to manually enable sandbox support.
// #define CEF_USE_SANDBOX 1

#if defined(CEF_USE_SANDBOX)
// The cef_sandbox.lib static library may not link successfully with all VS
// versions.
#pragma comment(lib, "cef_sandbox.lib")
#endif

namespace client {
	// tab-space: Name the namespace for prototyping.
	namespace RunMain {

		int RunMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow, TabSpaceState &tabSpaceState) {
			// Enable High-DPI support on Windows 7 or newer.
			CefEnableHighDPISupport();

			CefMainArgs main_args(hInstance);

			void *sandbox_info = NULL;

#if defined(CEF_USE_SANDBOX)
			// Manage the life span of the sandbox information object. This is necessary
			// for sandbox support on Windows. See cef_sandbox_win.h for complete details.
			CefScopedSandboxInfo scoped_sandbox;
			sandbox_info = scoped_sandbox.sandbox_info();
#endif

			// Parse command-line arguments.
			CefRefPtr<CefCommandLine> command_line = CefCommandLine::CreateCommandLine();
			command_line->InitFromString(::GetCommandLineW());

			// Create a ClientApp of the correct type.
			CefRefPtr<CefApp> app;
			ClientApp::ProcessType process_type = ClientApp::GetProcessType(command_line);
			if (process_type == ClientApp::BrowserProcess)
				app = new ClientAppBrowser();
			else if (process_type == ClientApp::RendererProcess)
				app = new ClientAppRenderer();
			else if (process_type == ClientApp::OtherProcess)
				app = new ClientAppOther();

			// Execute the secondary process, if any.
			// tab-space: Logging.
			// std::cout << "Executing any secondary processes..." << std::endl;
			int exit_code = CefExecuteProcess(main_args, app, sandbox_info);
			if (exit_code >= 0) {
				// tab-space: Logging.
				// std::cout << "Secondary process returned non-zero exit code." << std::endl;
				return exit_code;
			}

			// Create the main context object.
			// tab-space: Don't terminate CEF when all windows closed.
			MainContextImpl *context = new MainContextImpl(command_line, false);

			CefSettings settings;

#if !defined(CEF_USE_SANDBOX)
			settings.no_sandbox = true;
#endif

			// Applications should specify a unique GUID here to enable trusted downloads.
			CefString(&settings.application_client_id_for_file_scanning)
				.FromString("9A8DE24D-B822-4C6C-8259-5A848FEA1E68");

			// Populate the settings based on command line arguments.
			context->PopulateSettings(&settings);

			// Create the main message loop object.
			scoped_ptr<MainMessageLoop> message_loop;
			if (settings.multi_threaded_message_loop)
				message_loop.reset(new MainMessageLoopMultithreadedWin);
			else if (settings.external_message_pump)
				message_loop = MainMessageLoopExternalPump::Create();
			else
				message_loop.reset(new MainMessageLoopStd);

			// Initialize CEF.
			context->Initialize(main_args, settings, app, sandbox_info);

			// Register scheme handlers.
			test_runner::RegisterSchemeHandlers();

			// tab-space: Not launching root window, so don't need to build configuration here.
			//// tab-space: TODO: Create first window so that CEF doesn't shut down spuriously.
			//// tab-space: Logging.
			//std::cout << "Creating first window..." << std::endl;
			//RootWindowConfig window_config;
			//window_config.always_on_top = command_line->HasSwitch(switches::kAlwaysOnTop);
			//window_config.with_controls =
			//	!command_line->HasSwitch(switches::kHideControls);
			//window_config.with_osr = settings.windowless_rendering_enabled ? true : false;

			//// Create the first window.
			//// tab-space: This is only called once, even with multiple processes.
			//context->GetRootWindowManager()->CreateRootWindow(window_config);

			// tab-space: Context is now ready. Start the main logic thread.
			// tab-space: TODO: Why must the webserver thread be launched from here?
			tabSpaceState.context = context;
			tabSpaceState.mainLogicThread = std::thread([&]() {
				return tabSpaceState.mainLogicFunction(hInstance, hPrevInstance, lpCmdLine, nCmdShow, tabSpaceState);
				}
			);
			tabSpaceState.webserverThread = std::thread([&]() {
				return tabSpaceState.webserverFunction(tabSpaceState);
				}
			);

			// Run the message loop. This will block until Quit() is called by the
			// RootWindowManager after all windows have been destroyed.
			int result = message_loop->Run();

			/*// Create HWND to receive messages and process them in the main thread.
			std::unordered_map<UINT, Rain::RainWindow::MSGFC> msgm;
			msgm[WM_RAINAVAILABLE] = [](HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
				std::cout << "Received WM_RAINAVAILABLE on thread " << std::this_thread::get_id() << "..." << std::endl;

				TabInfo &tabInfo = *reinterpret_cast<TabInfo *>(wParam);

				std::this_thread::sleep_for(std::chrono::milliseconds(2000));
				tabInfo.hWnd = tabInfo.rootWindow->GetWindowHandle();
				std::cout << "New tab has HWND " << tabInfo.hWnd << ". " << tabInfo.rootWindow->GetBrowser() << std::endl;
				std::cout << "New tab has HWND " << tabInfo.hWnd << ". " << tabInfo.rootWindow->GetBrowser() << std::endl;
				RECT wndBounds;
				GetClientRect(tabInfo.hWnd, &wndBounds);
				tabInfo.left = wndBounds.left;
				tabInfo.top = wndBounds.top;
				tabInfo.width = wndBounds.right - wndBounds.left;
				tabInfo.height = wndBounds.bottom - wndBounds.top;

				// Get DC for entire desktop; but only capture the region containing the window.
				tabInfo.hdc = GetDC(tabInfo.hWnd);
				tabInfo.hDest = CreateCompatibleDC(tabInfo.hdc);
				tabInfo.hbmp = CreateCompatibleBitmap(tabInfo.hdc, tabInfo.width, tabInfo.height);
				SelectObject(tabInfo.hDest, tabInfo.hbmp);
				tabInfo.captureThread = std::thread([](TabInfo *pTabInfo) {
					pTabInfo->captureFunction();
					}, &tabInfo
				);
				tabInfo.captureThread.detach();

				return LRESULT(0);
			};
			tabSpaceState.mainRWnd.create(&msgm);

			// tab-space: Message loop.
			MSG msg;
			BOOL bRet;

			std::cout << "Running CEF message loop on thread " << std::this_thread::get_id() << "..." << std::endl;
			while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0) {
				if (bRet == -1)
					return -1; //serious error
				else {
					TranslateMessage(&msg);
					DispatchMessage(&msg);
					CefDoMessageLoopWork();
				}
			}*/

			// Shut down CEF.
			context->Shutdown();

			// Release objects in reverse order of creation.
			message_loop.reset();

			// tab-space: Since not using scoped_ptr, we must manually delete at the end of the scope.
			delete context;

			// tab-space: Wait for the main logic thread to terminate.
			tabSpaceState.webserverThread.join();
			tabSpaceState.mainLogicThread.join();

			return 0;
		}

	}  // namespace
}  // namespace client

// tab-space: Disabled for another entry point in main.
// Program entry point function.
// int APIENTRY wWinMain(HINSTANCE hInstance,
//   HINSTANCE hPrevInstance,
//   LPTSTR lpCmdLine,
//   int nCmdShow) {
//   UNREFERENCED_PARAMETER(hPrevInstance);
//   UNREFERENCED_PARAMETER(lpCmdLine);
//   return client::RunMain(hInstance, nCmdShow);
// }
