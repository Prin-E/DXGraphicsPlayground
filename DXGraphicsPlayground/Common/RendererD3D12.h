#pragma once

#include "RendererBase.h"
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

// Direct3D 12 Renderer base class.
class RendererD3D12 : public RendererBase
{
	// Member functions
public:
	RendererD3D12();
	~RendererD3D12();

	// Properties
	ID3D12Device* getDevice() const { return _device.Get(); }
	ID3D12CommandQueue* getQueue() const { return _queue.Get(); }
	virtual void setHWnd(HWND hWnd) override;

	// Device
	void createDevice();
	void cleanupDevice();

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

	// Member variables
private:
	// constants
	static constexpr int kFrameCount = 3;

	// Device
	ComPtr<IDXGIFactory4> _factory;
	ComPtr<IDXGIAdapter1> _currentAdapter;
	ComPtr<ID3D12Device> _device;

	// Queue
	ComPtr<ID3D12CommandQueue> _queue;
	ComPtr<ID3D12CommandAllocator> _renderCommandAllocators[kFrameCount];
	ComPtr<ID3D12GraphicsCommandList> _renderCommandLists[kFrameCount];

	// Swap chain
	ComPtr<IDXGISwapChain3> _swapChain;
	ComPtr<ID3D12Resource> _backBuffers[kFrameCount];
	ComPtr<ID3D12DescriptorHeap> _renderTargetViewHeap;
	ComPtr<ID3D12Fence> _fence;
	HANDLE _fenceEvent;
	UINT64 _fenceValues[kFrameCount];
	int _currentFrameIndex;
};