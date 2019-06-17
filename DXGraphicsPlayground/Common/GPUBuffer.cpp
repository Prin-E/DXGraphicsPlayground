#include "pch.h"
#include "GPUBuffer.h"
#include <cassert>
#include <iostream>

GPUBuffer::GPUBuffer(ID3D12Device* device, const size_t bufferSize, StorageMode storageMode) : GPUBuffer(device, bufferSize, 0, storageMode) { }

GPUBuffer::GPUBuffer(ID3D12Device* device, const size_t bufferSize, const size_t alignment, StorageMode storageMode) : _bufferPointer(nullptr), _open(false) {
	assert(device != nullptr && "Device is null.");

	// adjust buffer size with alignment
	_alignment = alignment;
	_alignedBufferSize = bufferSize;
	if (_alignment > 0)
		_alignedBufferSize = (bufferSize + _alignment - 1) & ~(_alignment - 1);

	HRESULT result = S_OK;

	D3D12_HEAP_PROPERTIES heapProps{};
	if (storageMode == StorageMode::Managed)
		heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	else
		heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProps.CreationNodeMask = 1;
	heapProps.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resourceDesc.Width = _alignedBufferSize;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	
	result = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&_buffer));
	_storageMode = storageMode;
}

GPUBuffer::~GPUBuffer() {
	close();
}

void GPUBuffer::setName(std::wstring& name) {
	_name = name;
	_buffer->SetName(_name.c_str());
}

bool GPUBuffer::open() {
	if (_storageMode == StorageMode::Private) {
		std::wcerr << L"Buffer can't be opened with private storage mode." << std::endl;
		return false;
	}

	HRESULT result = _buffer->Map(0, nullptr, &_bufferPointer);
	_open = result >= 0;
	if (!_open) {
		std::wcerr << L"Failed to open buffer " << _name << L"." << std::endl;
	}
	return _open;
}

void GPUBuffer::close() {
	if (_open) {
		D3D12_RANGE writeRange{ 0, _alignedBufferSize };
		_buffer->Unmap(0, &writeRange);
		_open = false;
	}
}

void GPUBuffer::copy(void* ptr, const size_t length, const size_t offset) {
	if (_open) {
		if (_alignedBufferSize < length + offset) {
			std::cerr << "Out of range (" << _alignedBufferSize << " < " + (length + offset) << ")." << std::endl;
			return;
		}

		memcpy(static_cast<UINT8*>(_bufferPointer) + offset, ptr, length);
	}
}