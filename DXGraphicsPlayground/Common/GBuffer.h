#pragma once

#include "pch.h"

using Microsoft::WRL::ComPtr;

class GBuffer
{
public:
	GBuffer(ID3D12Device* device, size_t newWidth = 800, size_t newHeight = 600);
	~GBuffer();

	inline ID3D12Resource* getAlbedo() const { return _albedo.Get(); }
	inline ID3D12Resource* getNormal() const { return _normal.Get(); }
	inline ID3D12Resource* getPos() const { return _pos.Get(); }
	inline ID3D12Resource* getShading() const { return _shading.Get(); }
	inline ID3D12Resource* getTangent() const { return _tangent.Get(); }

	inline size_t getWidth() const { return _width; }
	inline size_t getHeight() const { return _height; }

	inline ID3D12DescriptorHeap* getSRVDescriptorHeap() const { return _SRVDescriptorHeap.Get(); }
	inline ID3D12DescriptorHeap* getRTVDescriptorHeap() const { return _RTVDescriptorHeap.Get(); }

	inline ID3D12RootSignature* getGBufferRootSignature() const { return _gBufferRootSignature.Get(); }
	inline ID3D12RootSignature* getLightingRootSignature() const { return _lightingRootSignature.Get(); }

	void resize(size_t newWidth, size_t newHeight);

protected:
	void makeGBufferResources();
	void makeDescriptorHeaps();
	void makeRootSignatures();

	ID3D12Device* _device;

private:
	ComPtr<ID3D12Resource> _albedo;
	ComPtr<ID3D12Resource> _normal;		// world-space
	ComPtr<ID3D12Resource> _pos;		// world-space
	ComPtr<ID3D12Resource> _shading;	// R:roughness,G:metalic,BA:todo
	ComPtr<ID3D12Resource> _tangent;	// world-space

	ComPtr<ID3D12DescriptorHeap> _SRVDescriptorHeap;	// SRV
	ComPtr<ID3D12DescriptorHeap> _RTVDescriptorHeap;	// RTV

	ComPtr<ID3D12RootSignature> _gBufferRootSignature;
	ComPtr<ID3D12RootSignature> _lightingRootSignature;

	size_t _width, _height;
};