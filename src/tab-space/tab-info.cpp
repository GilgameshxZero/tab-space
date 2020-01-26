#include <Windows.h>
#include <iostream>

#include "gdi-plus-include.h"
#include "../tab-space/tab-info.h"

TabInfo::TabInfo() {
	this->width = 1920;
	this->height = 1080;
}

void TabInfo::captureFunction() {
	// Wait until we can get the browser window.
	this->browser = this->rootWindow->GetBrowser();
	while (this->browser == NULL) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		this->browser = this->rootWindow->GetBrowser();
	}
	
	this->hWnd = this->browser->GetHost()->GetWindowHandle();
	/*RECT wndBounds;
	GetWindowRect(this->hWnd, &wndBounds);
	std::cout << wndBounds.left << " " << wndBounds.top << " " << wndBounds.right << " " << wndBounds.bottom << std::endl;*/
	this->hdc = GetDC(this->hWnd);
	this->hDest = CreateCompatibleDC(this->hdc);
	this->hbmp = CreateCompatibleBitmap(this->hdc, this->width, this->height);
	SelectObject(this->hDest, this->hbmp);

	while (true) {
		/*GetWindowRect(this->hWnd, &wndBounds);
		BitBlt(this->hDest, 0, 0, this->width, this->height, this->hdc, wndBounds.left, wndBounds.top, SRCCOPY);*/
		PrintWindow(this->hWnd, this->hDest, 0x00000002);

		Gdiplus::Bitmap bmp(this->hbmp, (HPALETTE)0);
		bmp.Save(L"image.jpg", this->jpegClsid, NULL);

		/*std::cout << "Captured tab " << this->id << " with browser HWND " << this->hWnd << "." << std::endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(5000));*/
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	ReleaseDC(NULL, this->hdc);
	DeleteObject(this->hbmp);
	DeleteDC(this->hDest);
}
