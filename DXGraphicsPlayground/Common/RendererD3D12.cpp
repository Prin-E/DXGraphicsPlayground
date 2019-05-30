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
	IDXGIAdapter1* adapter = nullptr, * deviceAdapter = nullptr, * softwareAdapter = nullptr;
	for (int adapterIndex = 0; _factory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND; adapterIndex++) {
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			softwareAdapter = adapter;
		else if (deviceAdapter == nullptr)
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

	// frame buffers
	for (int i = 0; i < kFrameCount; i++) {
		// allocator
		_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_renderCommandAllocators[i]));

		// commnad list
		_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _renderCommandAllocators[i].Get(), nullptr, IID_PPV_ARGS(&_renderCommandLists[i]));
		_renderCommandLists[i]->Close();

		// fence
		_fenceValues[i] = 0;
	}
	
	// fence
	_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));
	_fenceEvent = CreateEvent(nullptr, false, false, nullptr);
}

void RendererD3D12::_cleanupDevice() {
	for (int i = 0; i < kFrameCount; i++) {
		if (_renderCommandLists[i].Get() != nullptr) {
			_renderCommandLists[i]->Release();
			_renderCommandLists[i] = nullptr;
		}
		if (_renderCommandAllocators[i].Get() != nullptr) {
			_renderCommandAllocators[i]->Release();
			_renderCommandAllocators[i] = nullptr;
		}
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
	_initBackBuffers();
}

void RendererD3D12::_initSwapChain() {
	if (_swapChain.Get() == nullptr) {
		HRESULT result = 0;
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.Width = 800;
		swapChainDesc.Height = 600;
		swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		swapChainDesc.BufferCount = kFrameCount;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
		if (_factory != nullptr) {
			ComPtr<IDXGISwapChain1> swapChain;
			result = _factory->CreateSwapChainForHwnd(_queue.Get(), _hWnd, &swapChainDesc, nullptr, nullptr, &swapChain);
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
	}

	if (_renderTargetViewHeap.Get() == nullptr) {
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = kFrameCount;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_renderTargetViewHeap));
	}
}

void RendererD3D12::_cleanupSwapChain() {
	for (int i = 0; i < kFrameCount; i++) {
		if (_backBuffers[i].Get() != nullptr) {
			_backBuffers[i]->Release();
			_backBuffers[i] = nullptr;
		}
	}

	if (_renderTargetViewHeap.Get() != nullptr) {
		_renderTargetViewHeap->Release();
		_renderTargetViewHeap = nullptr;
	}

	if (_swapChain.Get() != nullptr) {
		_swapChain->Release();
		_swapChain = nullptr;
	}
}

void RendererD3D12::_initBackBuffers() {
	if (_swapChain.Get() != nullptr) {
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = _renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();
		size_t rtvDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		for (int i = 0; i < kFrameCount; i++) {
			_swapChain->GetBuffer(i, IID_PPV_ARGS(&_backBuffers[i]));
			_device->CreateRenderTargetView(_backBuffers[i].Get(), nullptr, rtvHandle);
			rtvHandle.ptr += rtvDescriptorSize;
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
	auto commandAllocator = _renderCommandAllocators[_currentFrameIndex];
	auto commandList = _renderCommandLists[_currentFrameIndex];

	// Reset allocator and command list
	commandAllocator->Reset();
	commandList->Reset(commandAllocator.Get(), nullptr);

	// set ready to be a render target state
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.pResource = _backBuffers[_currentFrameIndex].Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	commandList->ResourceBarrier(1, &barrier);

	// Set render target
	size_t renderTargetViewSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = _renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += (renderTargetViewSize * _currentFrameIndex);
	commandList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);
	static float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	static float sign = 1.0f;
	clearColor[1] += 0.0166f * sign;
	if (clearColor[1] <= 0.0f || clearColor[1] >= 1.0f)
		sign *= -1;
}

void RendererD3D12::endFrame() {
	auto commandList = _renderCommandLists[_currentFrameIndex];

	// set ready to be a present state
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.pResource = _backBuffers[_currentFrameIndex].Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	commandList->ResourceBarrier(1, &barrier);
	commandList->Close();

	// Execute command lists
	ID3D12CommandList* commandLists[] = { commandList.Get() };
	_queue->ExecuteCommandLists(1, commandLists);

	// Swap buffers
	_swapChain->Present(1, 0);

	// Signal fence value
	const UINT64 currentFenceValue = _fenceValues[_currentFrameIndex];
	_queue->Signal(_fence.Get(), currentFenceValue);

	// Get next back buffer index
	_currentFrameIndex = _swapChain->GetCurrentBackBufferIndex();

	// Wait for fence value
	if (_fence->GetCompletedValue() < _fenceValues[_currentFrameIndex]) {
		_fence->SetEventOnCompletion(_fenceValues[_currentFrameIndex], _fenceEvent);
		WaitForSingleObjectEx(_fenceEvent, INFINITE, false);
	}
	_fenceValues[_currentFrameIndex] = currentFenceValue + 1;
}