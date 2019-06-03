#include "SimpleRenderer.h"
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <string>
#include <iostream>

using namespace DirectX;

static XMFLOAT3 kPos[] = {
	{ -0.5f, -0.5f,  0.0f },
	{  0.5f, -0.5f,  0.0f },
	{  0.0f,  0.5f,  0.0f }
};

void SimpleRenderer::init() {
	_initAssets();
}

void SimpleRenderer::_initAssets() {
	HRESULT result = S_OK;

	// Signature
	D3D12_ROOT_SIGNATURE_DESC signatureDesc = {};
	signatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	
	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	D3D12SerializeRootSignature(&signatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
	_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&_rootSignature));

	// Shader
	ComPtr<ID3DBlob> vertexShader, pixelShader;
	UINT compileFlags = 0;
#if defined(_DEBUG)
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
	constexpr DWORD rootPathSize = 512;
	WCHAR rootPath[rootPathSize] = {};
	GetModuleFileNameW(nullptr, rootPath, rootPathSize);
	WCHAR* rootPathLastSlash = wcsrchr(rootPath, L'\\');
	if (rootPathLastSlash)
		* (rootPathLastSlash + 1) = L'\0';

	std::wstring vertexShaderPath(rootPath), pixelShaderPath(rootPath);
	vertexShaderPath += L"VertexShader.cso";
	pixelShaderPath += L"PixelShader.cso";

	result = D3DReadFileToBlob(vertexShaderPath.c_str(), &vertexShader);
	if (result != S_OK) {
		std::wcout << L"Failed to load shader at " << vertexShaderPath.c_str() << std::endl;
		return;
	}
	result = D3DReadFileToBlob(pixelShaderPath.c_str(), &pixelShader);
	if (result != S_OK) {
		std::wcout << L"Failed to load shader at " << pixelShaderPath.c_str() << std::endl;
		return;
	}

	// Input element
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// Render Pipeline
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc = {};
	pipelineDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	pipelineDesc.pRootSignature = _rootSignature.Get();
	pipelineDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
	pipelineDesc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };
	pipelineDesc.SampleMask = UINT_MAX;
	pipelineDesc.NumRenderTargets = 1;
	pipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineDesc.SampleDesc.Count = 1;
	pipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	pipelineDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	result = _device->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(&_renderPipeline));

	// Vertex buffer
	D3D12_HEAP_PROPERTIES vertexBufferHeapProps = {};
	vertexBufferHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	vertexBufferHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	vertexBufferHeapProps.CreationNodeMask = 1;
	vertexBufferHeapProps.VisibleNodeMask = 1;
	D3D12_RESOURCE_DESC vertexBufferResourceDesc = {};
	vertexBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertexBufferResourceDesc.Width = sizeof(kPos);
	vertexBufferResourceDesc.Height = 1;
	vertexBufferResourceDesc.DepthOrArraySize = 1;
	vertexBufferResourceDesc.MipLevels = 1;
	vertexBufferResourceDesc.SampleDesc.Count = 1;
	vertexBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	result = _device->CreateCommittedResource(&vertexBufferHeapProps, D3D12_HEAP_FLAG_NONE, &vertexBufferResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&_vertexBuffer));

	UINT8* vertexBufferPointer = nullptr;
	D3D12_RANGE bufferReadRange = { 0, 0 }, bufferWriteRange = { 0, sizeof(kPos) };
	result = _vertexBuffer->Map(0, &bufferReadRange, reinterpret_cast<void**>(&vertexBufferPointer));
	memcpy(vertexBufferPointer, kPos, sizeof(kPos));
	_vertexBuffer->Unmap(0, nullptr);

	_vertexBufferView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
	_vertexBufferView.SizeInBytes = sizeof(kPos);
	_vertexBufferView.StrideInBytes = sizeof(XMFLOAT3);
}

void SimpleRenderer::_cleanupAssets() {
	// Because we use ComPtr reference, there's no need to release assets explicitly.
}

void SimpleRenderer::render() {
	auto commandList = _getRenderCommandList();
	D3D12_VIEWPORT viewport = { 0, 0, 800, 600 };
	D3D12_RECT scissorRect = { 0, 0, 800, 600 };
	
	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);
	commandList->SetGraphicsRootSignature(_rootSignature.Get());
	commandList->SetPipelineState(_renderPipeline.Get());
	commandList->IASetVertexBuffers(0, 1, &_vertexBufferView);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawInstanced(3, 1, 0, 0);

}