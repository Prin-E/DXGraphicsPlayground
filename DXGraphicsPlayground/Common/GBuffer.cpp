#include "pch.h"
#include "GBuffer.h"
#include "../Common/d3dx12.h"
#include <cassert>

GBuffer::GBuffer(ID3D12Device* device, size_t newWidth, size_t newHeight)
	: _device(device), _width(newWidth), _height(newHeight)
{
	assert(_device != nullptr && "Device is null.");

	makeGBufferResources();
	makeDescriptorHeaps();
}

GBuffer::~GBuffer() {
	// do nothing
}

void GBuffer::makeGBufferResources() {
	_albedo.Reset();
	_normal.Reset();
	_pos.Reset();
	_shading.Reset();
	_tangent.Reset();

	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.VisibleNodeMask = 1;
	heapProps.CreationNodeMask = 1;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Width = _width;
	resourceDesc.Height = _height;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	HRESULT result = S_OK;

	// albedo
	result = _device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET, nullptr, IID_PPV_ARGS(&_albedo));
	assert(result >= 0 && "Can't create Albedo G-buffer texture!");
	_albedo->SetName(L"Albedo G-buffer");

	// normal
	resourceDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	result = _device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET, nullptr, IID_PPV_ARGS(&_normal));
	assert(result >= 0 && "Can't create Normal G-buffer texture!");
	_normal->SetName(L"Normal G-buffer");

	// pos
	resourceDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	result = _device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET, nullptr, IID_PPV_ARGS(&_pos));
	assert(result >= 0 && "Can't create Position G-buffer texture!");
	_pos->SetName(L"Position G-buffer");

	// shading
	resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	result = _device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET, nullptr, IID_PPV_ARGS(&_shading));
	assert(result >= 0 && "Can't create Shading G-buffer texture!");
	_shading->SetName(L"Shading G-buffer");

	// tangent
	resourceDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	result = _device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET, nullptr, IID_PPV_ARGS(&_tangent));
	assert(result >= 0 && "Can't create Tangent G-buffer texture!");
	_tangent->SetName(L"Tangent G-buffer");
}

void GBuffer::makeDescriptorHeaps() {
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.NumDescriptors = 5;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask = 0;

	// SRV
	HRESULT result = S_OK;
	result = _device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_SRVDescriptorHeap));
	assert(result >= 0 && "Can't create SRV descriptor heap for G-buffer!");

	// Create shader-resource views
	size_t srvSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	auto srvHandle = _SRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	_device->CreateShaderResourceView(_albedo.Get(), nullptr, srvHandle); srvHandle.ptr += srvSize;
	_device->CreateShaderResourceView(_normal.Get(), nullptr, srvHandle); srvHandle.ptr += srvSize;
	_device->CreateShaderResourceView(_pos.Get(), nullptr, srvHandle); srvHandle.ptr += srvSize;
	_device->CreateShaderResourceView(_shading.Get(), nullptr, srvHandle); srvHandle.ptr += srvSize;
	_device->CreateShaderResourceView(_tangent.Get(), nullptr, srvHandle);

	// RTV
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	result = _device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_RTVDescriptorHeap));
	assert(result >= 0 && "Can't create RTV descriptor heap for G-buffer!");

	// Create render-target views
	size_t rtvSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	auto rtvHandle = _RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	_device->CreateRenderTargetView(_albedo.Get(), nullptr, rtvHandle); rtvHandle.ptr += rtvSize;
	_device->CreateRenderTargetView(_normal.Get(), nullptr, rtvHandle); rtvHandle.ptr += rtvSize;
	_device->CreateRenderTargetView(_pos.Get(), nullptr, rtvHandle); rtvHandle.ptr += rtvSize;
	_device->CreateRenderTargetView(_shading.Get(), nullptr, rtvHandle); rtvHandle.ptr += rtvSize;
	_device->CreateRenderTargetView(_tangent.Get(), nullptr, rtvHandle);
}

void GBuffer::makeRootSignatures() {
	ComPtr<ID3DBlob> rootSignatureBlob, errorBlob;
	HRESULT result = S_OK;

	CD3DX12_DESCRIPTOR_RANGE srvRanges{};
	CD3DX12_ROOT_PARAMETER params[5]{};
	CD3DX12_STATIC_SAMPLER_DESC samplers[8]{};
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc{};

	// G-buffer stage
	srvRanges.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 6, 0);	// albedo, normal, roughness, metalic, AO, anisotropic
	params[0].InitAsDescriptorTable(5, &srvRanges);
	params[1].InitAsConstantBufferView(0);
	params[2].InitAsConstantBufferView(1);
	rootSignatureDesc.Init(3, params, 0, nullptr);

	result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, rootSignatureBlob.GetAddressOf(), errorBlob.GetAddressOf());
	assert(result >= 0 && "Can't serialize root signature!");
	result = _device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&_gBufferRootSignature));
	assert(result >= 0 && "Can't create root signature!");

	// Lighting stage
	srvRanges.NumDescriptors = 8;	// 5 map + irradiance + prefiltered-specular + brdf lookup
	params[3].InitAsConstantBufferView(2);
	for (int i = 0; i < _countof(samplers); i++)
		samplers[i].Init(i);
	rootSignatureDesc.Init(4, params, 8, samplers);

	result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, rootSignatureBlob.GetAddressOf(), errorBlob.GetAddressOf());
	assert(result >= 0 && "Can't serialize root signature!");
	result = _device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&_lightingRootSignature));
	assert(result >= 0 && "Can't create root signature!");
}

void GBuffer::resize(size_t newWidth, size_t newHeight) {
	_width = newWidth;
	_height = newHeight;

	makeGBufferResources();
	makeDescriptorHeaps();
}