#pragma once

#include "pch.h"
#include <wrl.h>
#include <string>

using Microsoft::WRL::ComPtr;

enum class StorageMode {
	Private,
	Managed
};

class GPUBuffer {
public:
	GPUBuffer(ID3D12Device* device, const size_t bufferSize, StorageMode storageMode = StorageMode::Managed);
	GPUBuffer(ID3D12Device* device, const size_t bufferSize, const size_t alignment, StorageMode storageMode = StorageMode::Managed);
	~GPUBuffer();

	// Properties
	ID3D12Resource* getResource() const { return _buffer.Get(); }
	const size_t getAlignment() const { return _alignment; }
	const size_t getAlignedBufferSize() const { return _alignedBufferSize; }
	StorageMode getStorageMode() const { return _storageMode; }
	const std::wstring& getName() const { return _name; }
	void setName(std::wstring& name);

	// Data management
	bool open();
	void close();
	void copy(void* from, const size_t length, const size_t offset = 0);

private:
	ComPtr<ID3D12Resource> _buffer;
	void* _bufferPointer;
	size_t _alignment;
	size_t _alignedBufferSize;
	StorageMode _storageMode;
	std::wstring _name;
	bool _open;
};