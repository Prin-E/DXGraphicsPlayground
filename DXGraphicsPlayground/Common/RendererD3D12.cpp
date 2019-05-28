#include "pch.h"
#include "RendererD3D12.h"
#include <dxgi1_4.h>

RendererD3D12::RendererD3D12() {
	_initDevice();
}

void RendererD3D12::_initDevice() {
	ComPtr<IDXGIFactory4> factory = nullptr;
	HRESULT result = 0;

	CreateDXGIFactory2(0, IID_PPV_ARGS(factory.GetAddressOf()));
}
