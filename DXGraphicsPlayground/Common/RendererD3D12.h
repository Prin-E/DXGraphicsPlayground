#pragma once

#include "RendererBase.h"
#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

// Direct3D 12 Renderer base class.
class RendererD3D12 : public RendererBase
{
	// Member functions
public:
	RendererD3D12();
	~RendererD3D12();

private:
	void _initDevice();


	// Member variables
private:
	ComPtr<ID3D12Device> _device;
	ComPtr<ID3D12CommandQueue> _queue;
};