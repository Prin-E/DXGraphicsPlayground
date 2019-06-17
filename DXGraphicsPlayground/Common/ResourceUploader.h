#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <memory>

using Microsoft::WRL::ComPtr;

class GPUBuffer;
class ResourceUploader
{
public:
	ResourceUploader() {}
	~ResourceUploader() {}

	void updateSubresource(ID3D12GraphicsCommandList* commandList, UINT subresource, void* ptr, size_t length, ID3D12Resource* destinationTexture);

private:
	std::unique_ptr<GPUBuffer> _uploadBuffer;
};

