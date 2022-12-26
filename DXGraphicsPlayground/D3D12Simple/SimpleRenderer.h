#pragma once

#include "../Common/RendererD3D12.h"
#include "../Common/ResourceUploader.h"
#include "../Common/GPUBuffer.h"
#include <memory>

class SimpleRenderer : public RendererD3D12
{
public:
	virtual void update(float deltaTime) override;
	virtual void init() override;
	virtual void render() override;
	//virtual void resize(int newWidth, int newHeight) override;

protected:
	void _initAssets();
	void _cleanupAssets();
	void _initRootSignature();

private:
	ComPtr<ID3D12RootSignature> _rootSignature;
	ComPtr<ID3D12PipelineState> _renderPipeline;
	ComPtr<ID3D12Resource> _vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW _vertexBufferView;

	std::unique_ptr<GPUBuffer> _uniformBuffer;
	std::unique_ptr<GPUBuffer> _commonBuffer;

	ComPtr<ID3D12Resource> _texture;
	ComPtr<ID3D12DescriptorHeap> _textureSRVHeap;
};

