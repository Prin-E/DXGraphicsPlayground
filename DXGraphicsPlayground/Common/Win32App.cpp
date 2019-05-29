#include "pch.h"
#include "Win32App.h"
#include "RendererBase.h"
#include <cassert>

Win32App::Win32App(string& newTitle) :
	_title(newTitle), _hWnd(NULL), _renderThread(NULL), _renderer(NULL) {
	// set console encoding as UTF8
	SetConsoleOutputCP(CP_UTF8);
}

Win32App::~Win32App() {
	_renderer.reset();
}

HWND Win32App::createWindow(int width, int height) {
	constexpr size_t kMaxNameLength = 128;
	
	// string to TCHAR
	TCHAR className[kMaxNameLength] = {};
	int len = MultiByteToWideChar(CP_UTF8, 0, _title.c_str(), (int)_title.length(), NULL, NULL);
	MultiByteToWideChar(CP_UTF8, 0, _title.c_str(), (int)_title.length(), className, len);

	WNDCLASSEX wndClass = {};
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.lpszClassName = className;
	wndClass.lpszMenuName = NULL;
	wndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndClass.hIconSm = wndClass.hIcon;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hInstance = GetModuleHandle(NULL);
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = &Win32App::staticWndProc;
	RegisterClassEx(&wndClass);

	DWORD dwStyle = WS_OVERLAPPEDWINDOW;
	DWORD dwExStyle = WS_EX_WINDOWEDGE;
	HWND hWnd = CreateWindowEx(dwExStyle, className, className, dwStyle, 0, 0, width, height, NULL, NULL, wndClass.hInstance, NULL);

	// align window center
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	SetWindowPos(hWnd, NULL, (screenWidth - width) / 2, (screenHeight - height) / 2, width, height, 0);

	// additional data for window procdeure
	SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)this);

	_hWnd = hWnd;
	return hWnd;
}

void Win32App::show() {
	assert(_hWnd != NULL && "You must call createWindow() first!");
	ShowWindow(_hWnd, SW_SHOW);
}

int Win32App::messageLoop() {
	MSG msg = {};
	while (msg.message != WM_QUIT) {
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return 0;
}

LRESULT CALLBACK Win32App::staticWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	// find application object and call app-specific procedure.
	Win32App* app = reinterpret_cast<Win32App*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	if (app != nullptr) {
		return app->wndProc(hWnd, msg, wParam, lParam);
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK Win32App::wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_PAINT:
	{
		if (_renderer != nullptr) {
			_renderer->update(0.0166f);
			_renderer->render();
		}
		return 0;
	}
	case WM_SIZE:
	{
		if (_renderer != nullptr) {
			_renderer->resize(HIWORD(lParam), LOWORD(lParam));
		}
		return 0;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

void Win32App::setRenderer(RendererBase* newRenderer) {
	_renderer.reset(newRenderer);
}