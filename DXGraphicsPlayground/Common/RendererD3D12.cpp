#include "pch.h"
#include "RendererD3D12.h"
#include <iostream>

RendererD3D12::RendererD3D12() {
	createDevice();
}

RendererD3D12::~RendererD3D12() {
}

void RendererD3D12::createDevice() {
	if (_device.Get() == nullptr) {
		_initDevice();
	}
	else {
		std::cout << "Device is already created." << std::endl;
	}
}

void RendererD3D12::cleanupDevice() {
	if (_device.Get() == nullptr) {
		_cleanupDevice();
	}
	else {
		std::cout << "Device is already released." << std::endl;
	}
}

void RendererD3D12::_initDevice() {
	ComPtr<IDXGIFactory4> factory = nullptr;
	HRESULT result = 0;

	// Create factory
	UINT factoryFlags = 0;

#if defined(_DEBUG)
	ComPtr<ID3D12Debug> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		debugController->EnableDebugLayer();
		factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif
	result = CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&factory));
	if (result < 0) {
		std::cerr << "Failed to create DXGI Factory!" << std::endl;
	}
	_factory = factory;

	// Enumerate adapters 
	IDXGIAdapter1 *adapter = nullptr, *deviceAdapter = nullptr, *softwareAdapter = nullptr;
	for (int adapterIndex = 0; _factory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND; adapterIndex++) {
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			softwareAdapter = adapter;
		else if(deviceAdapter == nullptr)
			deviceAdapter = adapter;
	}

	// Create device
	if (deviceAdapter != nullptr) {
		result = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&_device));
		if (result >= 0)
			_currentAdapter = deviceAdapter;
	}
	if (deviceAdapter == nullptr || result < 0) {
		// Create software device (fallback)
		result = D3D12CreateDevice(softwareAdapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&_device));
		_currentAdapter = softwareAdapter;
	}

	// Print adapter information
	DXGI_ADAPTER_DESC1 desc;
	_currentAdapter->GetDesc1(&desc);

	std::cout << "Adapter information..." << std::endl;
	std::cout << "vendor : " << desc.VendorId << std::endl;
	std::cout << "device : " << desc.DeviceId << std::endl;
	std::wcout << "description : " << desc.Description << std::endl;
	std::cout << "system memory : " << desc.DedicatedSystemMemory << std::endl;
	std::cout << "video memory : " << desc.DedicatedVideoMemory << std::endl;
	std::cout << std::endl;

	// create render command queue
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_queue));

	// create render command allocator
	_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_renderCommandAllocator));
}

void RendererD3D12::_cleanupDevice() {
	if (_renderCommandAllocator.Get() != nullptr) {
		_renderCommandAllocator->Release();
		_renderCommandAllocator = nullptr;
	}

	if (_queue.Get() != nullptr) {
		_queue->Release();
		_queue = nullptr;
	}

	if (_device.Get() != nullptr) {
		_device->Release();
		_device = nullptr;
	}

	if (_currentAdapter.Get() != nullptr) {
		_currentAdapter->Release();
		_currentAdapter = nullptr;
	}

	if (_factory.Get() != nullptr) {
		_factory->Release();
	}
}

void RendererD3D12::setHWnd(HWND hWnd) {
	_cleanupSwapChain();
	RendererBase::setHWnd(hWnd);
	_initSwapChain();
}

void RendererD3D12::_initSwapChain() {
	if (_swapChain.Get() == nullptr) {
		HRESULT result = 0;
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.Width = 800;
		swapChainDesc.Height = 600;
		swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;
		swapChainDesc.BufferCount = 2;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
		if (_factory != nullptr) {
			ComPtr<IDXGISwapChain1> swapChain;
			result = _factory->CreateSwapChainForHwnd(_device.Get(), _hWnd, &swapChainDesc, nullptr, nullptr, &swapChain);
			if (result < 0) {
				std::cerr << "Failed to create DXGI Swap chain!" << std::endl;
				return;
			}
			result = swapChain.As(&_swapChain);
			if (result < 0) {
				std::cerr << "Failed to convert DXGI Swap chain revision 4!" << std::endl;
				return;
			}
			_currentFrameIndex = _swapChain->GetCurrentBackBufferIndex();
		}

		if (_renderTargetViewHeap.Get() != nullptr) {
			D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
			heapDesc.NumDescriptors = kFrameCount;
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_renderTargetViewHeap));
		}
	}
}

void RendererD3D12::_cleanupSwapChain() {
	if (_swapChain.Get() != nullptr) {
		_swapChain->Release();
		_swapChain = nullptr;
	}
}

void RendererD3D12::_initBackBuffers() {
	if (_swapChain.Get() != nullptr) {
		int renderTargetViewSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		for (int i = 0; i < kFrameCount; i++) {
			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = _renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();
			_swapChain->GetBuffer(0, IID_PPV_ARGS(&_backBuffers[i]));
			_device->CreateRenderTargetView(_backBuffers[i].Get(), nullptr, rtvHandle);
			rtvHandle.ptr += renderTargetViewSize * i;
		}
	}
}

void RendererD3D12::_cleanupBackBuffers() {

}

void RendererD3D12::update(float deltaTime) {

}

void RendererD3D12::render() {

}

void RendererD3D12::resize(int newWidth, int newHeight) {
	if (_swapChain != nullptr) {

	}
}

void RendererD3D12::beginFrame() {

}

void RendererD3D12::endFrame() {
	_swapChain->Present(1, 0);
}