#pragma once

#include <Windows.h>
#include <string>
#include <memory>

using namespace std;

class RendererBase;

class Win32App
{
public:

	Win32App(string &newTitle);
	~Win32App();

	// Window management
	HWND createWindow(int width, int height);
	void show();

	// Window procedure
	int messageLoop();

	// Renderer
	RendererBase* getRenderer() const { return _renderer.get(); }
	void setRenderer(RendererBase* newRenderer);

protected:
	static LRESULT CALLBACK staticWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	virtual LRESULT CALLBACK wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	constexpr static size_t kMaxNameLength = 128;

	string _title;
	HWND _hWnd;
	HANDLE _renderThread;		// TODO : separate render job from UI thread
	shared_ptr<RendererBase> _renderer;

	// application time
	LARGE_INTEGER _timePrev = {}, _timeCurrent = {}, _timeFrequency = {};
	float _timeSinceStartup = 0;
	float _deltaTime = 0;
	float _fps = 0;
	float _fpsCheck = 0;
};
