#pragma once

#include "../Common/RendererD3D12.h"

class SimpleRenderer : public RendererD3D12
{
public:
	virtual void init() override;
	virtual void render() override;
	//virtual void resize(int newWidth, int newHeight) override;

protected:
	void _initAssets();
	void _cleanupAssets();

private:
	ComPtr<ID3D12RootSignature> _rootSignature;
	ComPtr<ID3D12PipelineState> _renderPipeline;
	ComPtr<ID3D12Resource> _vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW _vertexBufferView;
};

