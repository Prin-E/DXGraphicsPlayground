#include "pch.h"
#include "RendererD3D12.h"
#include <iostream>

RendererD3D12::RendererD3D12() : _width(512), _height(512) {
	createDevice();
}

RendererD3D12::~RendererD3D12() {
	cleanupDevice();
}

void RendererD3D12::createDevice() {
	if (_device.Get() == nullptr) {
		_initDevice();
		_initFences();
	}
	else {
		std::cout << "Device is already created." << std::endl;
	}
}

void RendererD3D12::cleanupDevice() {
	if (_device.Get() != nullptr) {
		_waitForGpu();
		CloseHandle(_fenceEvent);
		// We don't need to release objects explicitly (ComPtrs do it automatically)
	}
	else {
		std::cout << "Device is already released." << std::endl;
	}
}

void RendererD3D12::init() {}

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
	IDXGIAdapter1* adapter = nullptr, *deviceAdapter = nullptr, * softwareAdapter = nullptr;
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
		D3D_FEATURE_LEVEL featureLevels[] = {
			D3D_FEATURE_LEVEL_12_0,
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0
		};

		for (int i = 0; i < _countof(featureLevels); i++) {
			result = D3D12CreateDevice(deviceAdapter, featureLevels[i], IID_PPV_ARGS(&_device));
			if (result >= 0) {
				_currentAdapter = deviceAdapter;
				_featureLevel = featureLevels[i];
				break;
			}
		}
	}
	if (deviceAdapter == nullptr || result < 0) {
		// Create software device (fallback)
		result = D3D12CreateDevice(softwareAdapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&_device));
		_currentAdapter = softwareAdapter;
		_featureLevel = D3D_FEATURE_LEVEL_12_0;
	}

	if (deviceAdapter != nullptr)
		deviceAdapter->Release();
	if (softwareAdapter != nullptr)
		softwareAdapter->Release();

	// Print adapter information
	DXGI_ADAPTER_DESC1 desc;
	_currentAdapter->GetDesc1(&desc);

	std::cout << "Feature Level : " << _featureLevel << std::endl;
	std::cout << "Adapter information..." << std::endl;
	std::cout << "vendor : " << desc.VendorId << std::endl;
	std::cout << "device : " << desc.DeviceId << std::endl;
	std::wcout << "description : " << desc.Description << std::endl;
	std::cout <<  "system memory : " << desc.DedicatedSystemMemory << " bytes" << std::endl;
	std::cout << "video memory : " << desc.DedicatedVideoMemory << " bytes" << std::endl;
	std::cout << std::endl;

	// create render command queue
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_queue));

	// frame buffers
	for (int i = 0; i < kMaxBuffersInFlight; i++) {
		// allocator
		_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_renderCommandAllocators[i]));

		// commnad list
		_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _renderCommandAllocators[i].Get(), nullptr, IID_PPV_ARGS(&_renderCommandLists[i]));
		_renderCommandLists[i]->Close();
	}
}

void RendererD3D12::_cleanupDevice() {
	for (int i = 0; i < kMaxBuffersInFlight; i++) {
		_renderCommandAllocators[i].Reset();
		_renderCommandLists[i].Reset();
	}

	_queue.Reset();
	_device.Reset();
	_currentAdapter.Reset();
	_factory.Reset();
}

void RendererD3D12::setHWnd(HWND hWnd) {
	_waitForGpu();
	_cleanupSwapChain();
	RendererBase::setHWnd(hWnd);
	_initSwapChain();
	_initBackBuffers();
}

void RendererD3D12::_initSwapChain() {
	if (_swapChain.Get() == nullptr) {
		HRESULT result = 0;
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.Width = _width;
		swapChainDesc.Height = _height;
		swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		swapChainDesc.BufferCount = kMaxBuffersInFlight;
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
		}
		_currentFrameIndex = _swapChain->GetCurrentBackBufferIndex();
	}

	if (_renderTargetViewHeap.Get() == nullptr) {
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = kMaxBuffersInFlight;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_renderTargetViewHeap));
	}
}

void RendererD3D12::_cleanupSwapChain() {
	_cleanupBackBuffers();
	_renderTargetViewHeap.Reset();
	_swapChain.Reset();
}

void RendererD3D12::_initBackBuffers() {
	if (_swapChain.Get() != nullptr) {
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = _renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();
		size_t rtvDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		for (int i = 0; i < kMaxBuffersInFlight; i++) {
			_swapChain->GetBuffer(i, IID_PPV_ARGS(&_backBuffers[i]));
			_device->CreateRenderTargetView(_backBuffers[i].Get(), nullptr, rtvHandle);
			rtvHandle.ptr += rtvDescriptorSize;
		}
	}
}

void RendererD3D12::_cleanupBackBuffers() {
	for (int i = 0; i < kMaxBuffersInFlight; i++) {
		_backBuffers[i].Reset();
		_fenceValues[i] = _fenceValues[_currentFrameIndex];
	}
}

void RendererD3D12::_initFences() {
	if (_fence.Get() == nullptr) {
		for (int i = 0; i < kMaxBuffersInFlight; i++) {
			_fenceValues[i] = 0;
		}

		// fence
		_device->CreateFence(_fenceValues[_currentFrameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));
		_fenceValues[_currentFrameIndex]++;
		_fenceEvent = CreateEvent(nullptr, false, false, nullptr);
	}
}

void RendererD3D12::_waitForGpu() {
	// Signal fence value
	if (_queue != nullptr && _fence != nullptr) {
		const UINT64 currentFenceValue = _fenceValues[_currentFrameIndex];
		_queue->Signal(_fence.Get(), currentFenceValue);

		// Wait for completion
		_fence->SetEventOnCompletion(currentFenceValue, _fenceEvent);
		WaitForSingleObjectEx(_fenceEvent, INFINITE, false);
		_fenceValues[_currentFrameIndex] = currentFenceValue + 1;
	}
}

void RendererD3D12::_prepareNextBackBuffer() {
	// Signal fence value
	const UINT64 currentFenceValue = _fenceValues[_currentFrameIndex];
	_queue->Signal(_fence.Get(), currentFenceValue);

	// Get next back buffer index
	_currentFrameIndex = _swapChain->GetCurrentBackBufferIndex();

	// Wait for fence value
	UINT64 completedFenceValue = _fence->GetCompletedValue();
	if (_fence->GetCompletedValue() < _fenceValues[_currentFrameIndex]) {
		_fence->SetEventOnCompletion(_fenceValues[_currentFrameIndex], _fenceEvent);
		WaitForSingleObjectEx(_fenceEvent, INFINITE, false);
	}
	_fenceValues[_currentFrameIndex] = currentFenceValue + 1;
}

void RendererD3D12::update(float deltaTime) {}

void RendererD3D12::render() {}

void RendererD3D12::resize(int newWidth, int newHeight) {
	if (_swapChain != nullptr) {
		_waitForGpu();
		_cleanupBackBuffers();

		_swapChain->ResizeBuffers(kMaxBuffersInFlight, newWidth, newHeight, DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
		_currentFrameIndex = _swapChain->GetCurrentBackBufferIndex();

		_initBackBuffers();
		_width = newWidth;
		_height = newHeight;
	}
}

void RendererD3D12::beginFrame() {
	auto commandAllocator = _renderCommandAllocators[_currentFrameIndex];
	auto commandList = _renderCommandLists[_currentFrameIndex];
	HRESULT result = S_OK;

	// Reset allocator and command list
	result = commandAllocator->Reset();
	result = commandList->Reset(commandAllocator.Get(), nullptr);

	// set ready to be a render target state
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.pResource = _backBuffers[_currentFrameIndex].Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	commandList->ResourceBarrier(1, &barrier);

	// Set viewport rect
	D3D12_VIEWPORT viewport = { 0, 0, static_cast<float>(_width), static_cast<float>(_height) };
	D3D12_RECT scissorRect = { 0, 0, _width, _height };
	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);

	// Set render target
	size_t renderTargetViewSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = _renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += (renderTargetViewSize * _currentFrameIndex);
	commandList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);
	static float clearColor[4] = { 0.2f, 0.2f, 0.2f, 0.0f };
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
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

	// Prepare next backbuffer...
	_prepareNextBackBuffer();
}