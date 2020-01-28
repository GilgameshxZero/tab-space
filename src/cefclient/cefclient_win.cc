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

#include "../tab-space/state.h"

// tab-space: Sandbox is manually turned off; i.e. CEF_USE_SANDBOX is not defined.
#ifdef CEF_USE_SANDBOX
#undef CEF_USE_SANDBOX
#endif

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

		int RunMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow, TabSpace::State &state) {
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

			// tab-space: TODO: Why doesn't this work? DPI support.
			command_line->AppendSwitchWithValue("high-dpi-support", "1");
			command_line->AppendSwitchWithValue("force-device-scale-factor", "1");

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
			int exit_code = CefExecuteProcess(main_args, app, sandbox_info);
			if (exit_code >= 0) {
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
				.FromString("7de15061-6d69-4cdb-9023-5fc8b98c5c06");

			// Populate the settings based on command line arguments.
			context->PopulateSettings(&settings);

			// tab-space: Don't clutter stdout.
			settings.log_severity = cef_log_severity_t::LOGSEVERITY_DISABLE;

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
			/*RootWindowConfig window_config;
			window_config.always_on_top = command_line->HasSwitch(switches::kAlwaysOnTop);
			window_config.with_controls =
				!command_line->HasSwitch(switches::kHideControls);
			window_config.with_osr = settings.windowless_rendering_enabled ? true : false;

			// Create the first window.
			context->GetRootWindowManager()->CreateRootWindow(window_config);*/

			// tab-space: Context is now ready. Start the main logic thread.
			state.context = context;
			state.tabSpaceThread = std::thread([&]() {
				return state.tabSpaceThreadMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow, state);
			});

			// Run the message loop. This will block until Quit() is called by the
			// RootWindowManager after all windows have been destroyed.
			int result = message_loop->Run();

			// tab-space: Wait for the main logic thread to terminate.
			state.tabSpaceThread.join();

			// Shut down CEF.
			context->Shutdown();

			// Release objects in reverse order of creation.
			message_loop.reset();

			// tab-space: Since not using scoped_ptr, we must manually delete at the end of the scope.
			delete context;

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
