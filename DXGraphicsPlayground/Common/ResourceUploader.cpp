#include "pch.h"
#include "ResourceUploader.h"
#include "GPUBuffer.h"
#include <cassert>

void ResourceUploader::updateSubresource(ID3D12GraphicsCommandList* commandList, UINT subresource, void* ptr, size_t length, ID3D12Resource* destinationTexture) {
	assert(destinationTexture != nullptr && "Destination resource is null.");

	// Get device object from texture 
	ComPtr<ID3D12Device> device = nullptr;
	destinationTexture->GetDevice(IID_PPV_ARGS(&device));

	// Query minimum required intermediate buffer size
	size_t intermediateBufferSize = 0;
	D3D12_RESOURCE_DESC destDesc = destinationTexture->GetDesc();
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT destFootprint{};
	device->GetCopyableFootprints(&destDesc, 0, 1, 0, &destFootprint, nullptr, nullptr, &intermediateBufferSize);

	if (_uploadBuffer == nullptr || _uploadBuffer->getAlignedBufferSize() < intermediateBufferSize) {
		_uploadBuffer = std::make_unique<GPUBuffer>(device.Get(), intermediateBufferSize);
	}

	if (_uploadBuffer->open()) {
		if (intermediateBufferSize == length) {
			_uploadBuffer->copy(ptr, length, 0);
		}
		else {
			// texture's row pitch is different with buffer's row pitch. so we need to copy each rows manually.
			size_t srcRowPitch = length / destFootprint.Footprint.Height;
			size_t rowPitch = destFootprint.Footprint.RowPitch;
			for (UINT i = 0; i < destFootprint.Footprint.Height; i++) {
				_uploadBuffer->copy(reinterpret_cast<UINT8*>(ptr) + srcRowPitch * i, srcRowPitch, rowPitch * i);
			}
		}
		_uploadBuffer->close();

		// common->copy dest
		D3D12_RESOURCE_BARRIER destBarrier{};
		destBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		destBarrier.Transition.pResource = destinationTexture;
		destBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
		destBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
		destBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		commandList->ResourceBarrier(1, &destBarrier);

		D3D12_TEXTURE_COPY_LOCATION srcLoc{};
		srcLoc.pResource = _uploadBuffer->getResource();
		srcLoc.PlacedFootprint = destFootprint;
		srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		D3D12_TEXTURE_COPY_LOCATION destLoc{};
		destLoc.pResource = destinationTexture;
		destLoc.SubresourceIndex = 0;
		destLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

		if (destDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
			commandList->CopyBufferRegion(destinationTexture, 0, _uploadBuffer->getResource(), 0, length);
		}
		else {
			commandList->CopyTextureRegion(&destLoc, 0, 0, 0, &srcLoc, nullptr);
		}

		// copy dest->pixel shader resource
		destBarrier.Transition.StateBefore = destBarrier.Transition.StateAfter;
		destBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		commandList->ResourceBarrier(1, &destBarrier);
	}
}