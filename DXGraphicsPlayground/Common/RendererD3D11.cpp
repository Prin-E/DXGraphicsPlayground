#include "pch.h"
#include "RendererD3D11.h"
#include "D3DInternalUtils.h"
#include <iostream>

RendererD3D11::RendererD3D11() : _width(512), _height(512) {
	createDevice();
}

RendererD3D11::~RendererD3D11() {
	cleanupDevice();
}

void RendererD3D11::createDevice() {
	if (_device.Get() == nullptr) {
		_initDevice();
		_initDefaultAssets();
	}
	else {
		std::cout << "Device is already created." << std::endl;
	}
}

void RendererD3D11::cleanupDevice() {
	if (_device.Get() != nullptr) {
		// We don't need to release objects explicitly (ComPtrs do it automatically)
	}
	else {
		std::cout << "Device is already released." << std::endl;
	}
}

void RendererD3D11::init() {}

void RendererD3D11::displayDidChange() {}

void RendererD3D11::_initDefaultAssets() {
	HRESULT result = S_OK;
	
	// Depth Stencil
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.StencilEnable = false;
	depthStencilDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	depthStencilDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

	result = _device->CreateDepthStencilState(&depthStencilDesc, &_depthStencilState);
	if (result != S_OK) {
		std::cerr << "Failed to create depth stencil state!" << std::endl;
		return;
	}
}

void RendererD3D11::_initDevice() {
	ComPtr<IDXGIFactory1> factory = nullptr;
	HRESULT result = 0;

	// Create factory
	UINT factoryFlags = 0;

#if defined(_DEBUG)
	factoryFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	result = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
	if (result < 0) {
		std::cerr << "Failed to create DXGI Factory!" << std::endl;
	}
	_factory = factory;

	// Enumerate adapters
	IDXGIAdapter1* adapter = nullptr, *deviceAdapter = nullptr, *softwareAdapter = nullptr;
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
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0
		};

		result = D3D11CreateDevice(deviceAdapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, factoryFlags, featureLevels, _countof(featureLevels), D3D11_SDK_VERSION, &_device, &_featureLevel, &_context);
		if (result >= 0) {
			_currentAdapter = deviceAdapter;
		}
		if (deviceAdapter == nullptr || result < 0) {
			// Create software device (fallback)
			result = D3D11CreateDevice(softwareAdapter, D3D_DRIVER_TYPE_SOFTWARE, GetModuleHandle(NULL), factoryFlags, NULL, D3D_FEATURE_LEVEL_11_0, 0, &_device, NULL, &_context);
			_currentAdapter = softwareAdapter;
			_featureLevel = D3D_FEATURE_LEVEL_11_0;
		}

		if (deviceAdapter != nullptr)
			deviceAdapter->Release();
		if (softwareAdapter != nullptr)
			softwareAdapter->Release();

		// Print adapter information
		DXGI_ADAPTER_DESC1 desc;
		_currentAdapter->GetDesc1(&desc);

		struct thousands_sep : std::numpunct<char> {
			char do_thousands_sep()   const { return ','; }
			std::string do_grouping() const { return "\3"; }
		};
		std::ios oldstate{ nullptr };
		oldstate.copyfmt(std::cout);
		std::cout << "Feature Level : " << FeatureLevelToString(_featureLevel) << std::endl;
		std::cout << "Adapter Info" << std::endl;

		std::cout << std::hex;
		std::cout << "- vendor : 0x" << desc.VendorId << std::endl;
		std::cout << "- device : 0x" << desc.DeviceId << std::endl;
		std::wcout << "- description : " << desc.Description << std::endl;

		std::cout.imbue(std::locale(std::cout.getloc(), new thousands_sep));
		std::cout << std::dec;
		std::cout << "- system memory : " << std::dec << desc.DedicatedSystemMemory << " bytes" << std::endl;
		std::cout << "- video memory : " << desc.DedicatedVideoMemory << " bytes" << std::endl;
		std::cout << std::endl;
		std::cout.copyfmt(oldstate);
	}
}

void RendererD3D11::_cleanupDevice() {
	_device.Reset();
	_context.Reset();
	_currentAdapter.Reset();
	_factory.Reset();
}

void RendererD3D11::setHWnd(HWND hWnd) {
	_cleanupSwapChain();
	RendererBase::setHWnd(hWnd);
	_initSwapChain();
	_initBackBuffers();
}

void RendererD3D11::_initSwapChain() {
	if (_swapChain.Get() == nullptr) {
		HRESULT result = 0;
		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		swapChainDesc.BufferDesc.Width = _width;
		swapChainDesc.BufferDesc.Height = _height;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		swapChainDesc.BufferCount = kMaxBuffersInFlight;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.Windowed = true;
		swapChainDesc.OutputWindow = _hWnd;
		if (swapChainDesc.BufferCount == 1)
			swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		else
			swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		if (_factory != nullptr) {
			ComPtr<IDXGISwapChain> swapChain;
			result = _factory->CreateSwapChain(_device.Get(), &swapChainDesc, &swapChain);
			if (result < 0) {
				std::cerr << "Failed to create DXGI Swap Chain!" << std::endl;
				return;
			}
			_swapChain = swapChain;
		}
	}
}

void RendererD3D11::_cleanupSwapChain() {
	_cleanupBackBuffers();
	_swapChain.Reset();
}

void RendererD3D11::_initBackBuffers() {
	if (_swapChain.Get() != nullptr) {
		HRESULT result = S_OK;
		for (int i = 0; i < kMaxBuffersInFlight; i++) {
			ID3D11Texture2D *backBuffer = nullptr;
			result = _swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));
			if (result == S_OK) {
				D3D11_RENDER_TARGET_VIEW_DESC desc{};
				desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
				desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
				desc.Texture2D.MipSlice = 0;
				result = _device->CreateRenderTargetView(backBuffer, &desc, &_renderTargetViews[i]);
				if (result != S_OK) {
					std::cerr << "Failed to create render target view #" << i << "!" << std::endl;
				}
				backBuffer->Release();
			}
			else {
				std::cerr << "Failed to query back buffer #" << i << "!" << std::endl;
			}
		}

		// Depth Stencil
		D3D11_TEXTURE2D_DESC depthStencilTextureDesc = {};
		depthStencilTextureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthStencilTextureDesc.Usage = D3D11_USAGE_DEFAULT;
		depthStencilTextureDesc.Width = _width;
		depthStencilTextureDesc.Height = _height;
		depthStencilTextureDesc.ArraySize = 1;
		depthStencilTextureDesc.SampleDesc.Count = 1;
		depthStencilTextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		result = _device->CreateTexture2D(&depthStencilTextureDesc, nullptr, &_depthStencilTexture);
		if (result != S_OK) {
			std::cerr << "Failed to create depth stencil texture!" << std::endl;
		}

		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
		depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		result = _device->CreateDepthStencilView(_depthStencilTexture.Get(), &depthStencilViewDesc, &_depthStencilView);
		if (result != S_OK) {
			std::cerr << "Failed to create depth stencil view!" << std::endl;
		}
	}
}

void RendererD3D11::_cleanupBackBuffers() {
	for (int i = 0; i < kMaxBuffersInFlight; i++) {
		_renderTargetViews[i].Reset();
	}
	_depthStencilView.Reset();
	_depthStencilTexture.Reset();
}

void RendererD3D11::update(float deltaTime) {}

void RendererD3D11::render() {}

void RendererD3D11::move(int windowX, int windowY) {}

void RendererD3D11::resize(int newWidth, int newHeight) {
	if (_swapChain != nullptr) {
		_cleanupBackBuffers();

		_width = newWidth;
		_height = newHeight;
		_swapChain->ResizeBuffers(kMaxBuffersInFlight, newWidth, newHeight, DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
		_initBackBuffers();
	}
}

void RendererD3D11::beginFrame() {
	// Set viewport rect
	D3D11_VIEWPORT viewport = { 0, 0, static_cast<float>(_width), static_cast<float>(_height), 0, 1 };
	D3D11_RECT scissorRect = { 0, 0, _width, _height };
	_context->RSSetViewports(1, &viewport);
	_context->RSSetScissorRects(1, &scissorRect);

	// Set render target
	_context->OMSetRenderTargets(1, _renderTargetViews[_currentFrameIndex].GetAddressOf(), _depthStencilView.Get());
	_context->OMSetDepthStencilState(_depthStencilState.Get(), 0);
	static float clearColor[4] = { 0.2f, 0.2f, 0.2f, 0.0f };
	_context->ClearRenderTargetView(_renderTargetViews[_currentFrameIndex].Get(), clearColor);
	_context->ClearDepthStencilView(_depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void RendererD3D11::endFrame() {
	_swapChain->Present(1, 0);
	_currentFrameIndex = (_currentFrameIndex + 1) % kMaxBuffersInFlight;
}