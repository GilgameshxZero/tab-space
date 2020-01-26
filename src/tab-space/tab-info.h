#pragma once

#include <Windows.h>
#include <string>
#include <thread>

#include "../cefclient/browser/main_context_impl.h"

// Everything we need to know about a tab.
class TabInfo {
public:
	TabInfo();

	CLSID *jpegClsid;

	std::string id;
	int left, top, width, height;

	scoped_refptr<client::RootWindow> rootWindow;
	scoped_refptr<CefBrowser> browser;

	HWND hWnd;
	HDC hdc, hDest;
	HBITMAP hbmp;

	std::thread captureThread;
	void captureFunction();

	// Most recent JPEG capture of the window.
};
