#pragma once

#include "RendererBase.h"
#include <dxgidebug.h>
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

// Direct3D 11 Renderer base class.
class RendererD3D11 : public RendererBase
{
	/* Member functions */
public:
	RendererD3D11();
	~RendererD3D11();

	// Properties
	ID3D11Device* getDevice() const { return _device.Get(); }
	virtual void setHWnd(HWND hWnd) override;

	// Device
	void createDevice();
	void cleanupDevice();

	// Initialization
	virtual void init() override;

	// Rendering
	virtual void update(float deltaTime) override;
	virtual void render() override;
	virtual void resize(int newWidth, int newHeight) override;
	virtual void beginFrame() override;
	virtual void endFrame() override;

private:
	void _initDevice();
	void _cleanupDevice();

	void _initSwapChain();
	void _cleanupSwapChain();
	void _initBackBuffers();
	void _cleanupBackBuffers();

	/* Member variables */
protected:
	// constants
	static constexpr int kMaxBuffersInFlight = 1;

	// Device
	ComPtr<IDXGIFactory1> _factory;
	ComPtr<IDXGIAdapter1> _currentAdapter;
	ComPtr<ID3D11Device> _device;
	D3D_FEATURE_LEVEL _featureLevel;

	// Context
	ComPtr<ID3D11DeviceContext> _context;

	// Swap Cahin
	ComPtr<IDXGISwapChain> _swapChain;
	ComPtr<ID3D11RenderTargetView> _renderTargetViews[kMaxBuffersInFlight];
	int _width, _height;
	int _currentFrameIndex;
};

