#include <Windows.h>
#include <iostream>

#include "../rain-library-4/gdi-plus-include.h"

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

	this->data = new char[1000000];

	while (true) {
		/*GetWindowRect(this->hWnd, &wndBounds);
		BitBlt(this->hDest, 0, 0, this->width, this->height, this->hdc, wndBounds.left, wndBounds.top, SRCCOPY);*/

		// TODO: 0x00000002 is very necessary for capturing GPU renders.
		PrintWindow(this->hWnd, this->hDest, 0x00000002);

		// Keep the bitmap.
		Gdiplus::Bitmap bmp(this->hbmp, (HPALETTE)0);
		IStream *istream = nullptr;
		HRESULT hr = CreateStreamOnHGlobal(NULL, TRUE, &istream);
		bmp.Save(istream, this->jpegClsid);

		HGLOBAL hg = NULL;
		GetHGlobalFromStream(istream, &hg);

		//copy IStream to buffer
		int bufsize = GlobalSize(hg);

		//lock & unlock memory
		LPVOID pimage = GlobalLock(hg);
		memcpy(this->data, pimage, bufsize);
		this->bufsize = bufsize;
		GlobalUnlock(hg);

		istream->Release();
		// std::cout << bufsize << std::endl;

		// bmp.Save(L"image.jpg", this->jpegClsid, NULL);

		/*std::cout << "Captured tab " << this->id << " with browser HWND " << this->hWnd << "." << std::endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(5000));*/
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	ReleaseDC(NULL, this->hdc);
	DeleteObject(this->hbmp);
	DeleteDC(this->hDest);
}
