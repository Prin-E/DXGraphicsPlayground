#pragma once

#include <Windows.h>

// Renderer base class
class RendererBase
{
public:
	RendererBase() : _hWnd(0) {}
	virtual ~RendererBase() {}

	HWND getHWnd() const { return _hWnd; }
	virtual void setHWnd(HWND hWnd) { _hWnd = hWnd; }

	virtual void init() = 0;

	virtual void update(float deltaTime) = 0;
	virtual void render() = 0;
	virtual void resize(int newWidth, int newHeight) = 0;

	virtual void beginFrame() = 0;
	virtual void endFrame() = 0;

protected:
	// window handle
	HWND _hWnd;
};

