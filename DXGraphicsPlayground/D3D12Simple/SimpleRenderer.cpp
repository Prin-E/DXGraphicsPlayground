#include "SimpleRenderer.h"
#include "../Common/Time.h"
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <string>
#include <iostream>
#include <pix3.h>

#define STB_IMAGE_IMPLEMENTATION 1
#include "stb_image.h"

using namespace DirectX;

struct VertexInfo {
	XMFLOAT3 pos;
	XMFLOAT3 color;
	XMFLOAT2 uv;
};

static VertexInfo kVertices[] = {
	{ XMFLOAT3(-0.5f, -0.5f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
	{ XMFLOAT3(-0.5f,  0.5f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f)  },
	{ XMFLOAT3( 0.5f, -0.5f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 0.0f)  },
	{ XMFLOAT3( 0.5f, -0.5f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 0.0f)  },
	{ XMFLOAT3(-0.5f,  0.5f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f)  },
	{ XMFLOAT3( 0.5f,  0.5f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 1.0f)  },
};

_declspec(align(256)) struct CommonInfo {
	float normalizedSDRWhiteLevel;
	bool isST2084Output;
};

_declspec(align(256)) struct ObjectInfo {
	XMMATRIX view;
	XMMATRIX projection;
};

void SimpleRenderer::init() {
	_initAssets();
}

void SimpleRenderer::_initAssets() {
	HRESULT result = S_OK;

	// Prepare command list for initialize
	auto commandList = _getRenderCommandList();
	auto commandAllocator = _getRenderCommandAllocator();
	commandAllocator->Reset();
	commandList->Reset(_getRenderCommandAllocator(), nullptr);

	// Root signature
	_initRootSignature();

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
	vertexShaderPath += L"VertexShader_D3D12Simple.cso";
	pixelShaderPath += L"PixelShader_D3D12Simple.cso";

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
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(XMFLOAT3), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(XMFLOAT3) * 2, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// Render Pipeline
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc = {};
	pipelineDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	pipelineDesc.pRootSignature = _rootSignature.Get();
	pipelineDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
	pipelineDesc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };
	pipelineDesc.SampleMask = UINT_MAX;
	pipelineDesc.NumRenderTargets = 1;
	pipelineDesc.RTVFormats[0] = DXGI_FORMAT_B8G8R8A8_UNORM;
	pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineDesc.SampleDesc.Count = 1;
	pipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	pipelineDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	pipelineDesc.RasterizerState.DepthClipEnable = true;
	pipelineDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	pipelineDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
	pipelineDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	pipelineDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	pipelineDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	pipelineDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	pipelineDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
	pipelineDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
	result = _device->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(&_renderPipeline));
	if (result != S_OK) {
		std::cout << "Failed to create pipeline state! : " << result << std::endl;
	}

	// Vertex buffer
	D3D12_HEAP_PROPERTIES vertexBufferHeapProps = {};
	vertexBufferHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	vertexBufferHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	vertexBufferHeapProps.CreationNodeMask = 1;
	vertexBufferHeapProps.VisibleNodeMask = 1;
	D3D12_RESOURCE_DESC vertexBufferResourceDesc = {};
	vertexBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertexBufferResourceDesc.Width = sizeof(kVertices);
	vertexBufferResourceDesc.Height = 1;
	vertexBufferResourceDesc.DepthOrArraySize = 1;
	vertexBufferResourceDesc.MipLevels = 1;
	vertexBufferResourceDesc.SampleDesc.Count = 1;
	vertexBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	result = _device->CreateCommittedResource(&vertexBufferHeapProps, D3D12_HEAP_FLAG_NONE, &vertexBufferResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&_vertexBuffer));
	if (result != S_OK) {
		std::cout << "Failed to create committed resource! : " << result << std::endl;
	}

	UINT8* vertexBufferPointer = nullptr;
	D3D12_RANGE bufferReadRange = { 0, 0 }, bufferWriteRange = { 0, sizeof(kVertices) };
	result = _vertexBuffer->Map(0, &bufferReadRange, reinterpret_cast<void**>(&vertexBufferPointer));
	if (result != S_OK) {
		std::cout << "Failed to copy vertex data! : " << result << std::endl;
	}
	memcpy(vertexBufferPointer, kVertices, sizeof(kVertices));
	_vertexBuffer->Unmap(0, nullptr);

	_vertexBufferView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
	_vertexBufferView.SizeInBytes = sizeof(kVertices);
	_vertexBufferView.StrideInBytes = sizeof(VertexInfo);

	// CBVs
	_commonBuffer = std::make_unique<GPUBuffer>(_device.Get(), sizeof(CommonInfo) * kMaxBuffersInFlight);
	_uniformBuffer = std::make_unique<GPUBuffer>(_device.Get(), sizeof(ObjectInfo) * kMaxBuffersInFlight);

	// Map buffer pointer (we don't unmap until app closes)
	_commonBuffer->open();
	_uniformBuffer->open();

	// image flag
	// NOTE: set TRUE to load image from file!
	constexpr bool useSTBImage = false;

	// STB image
	struct
	{
		int x, y, channel;
	} imginfo{};
	stbi_set_flip_vertically_on_load_thread(TRUE);
	stbi_uc* stbimg = nullptr;
	if (useSTBImage) {
		stbimg = stbi_load("../Assets/Textures/PrinE2013.jpg", &imginfo.x, &imginfo.y, &imginfo.channel, 4);
	}

	// checkbox pattern
	constexpr size_t checkboxSize = 128;
	constexpr size_t checkboxCellWidth = 8;
	const size_t checkboxTextureSize = checkboxSize * checkboxSize * 4;
	UINT8* checkboxTexturePointer = new UINT8[checkboxTextureSize];
	for (int j = 0; j < checkboxSize; j++) {
		for (int i = 0; i < checkboxSize; i++) {
			checkboxTexturePointer[(j * checkboxSize + i) * 4] = (i / checkboxCellWidth + j / checkboxCellWidth) % 2 ? 255 : 0;
			checkboxTexturePointer[(j * checkboxSize + i) * 4 + 1] = (i / checkboxCellWidth + j / checkboxCellWidth) % 2 ? 255 : 0;
			checkboxTexturePointer[(j * checkboxSize + i) * 4 + 2] = (i / checkboxCellWidth + j / checkboxCellWidth) % 2 ? 255 : 0;
			checkboxTexturePointer[(j * checkboxSize + i) * 4 + 3] = 255;
		}
	}

	UINT textureWidth = checkboxSize, textureHeight = checkboxSize;
	if (stbimg != nullptr) {
		textureWidth = imginfo.x;
		textureHeight = imginfo.y;
	}

	// texture
	D3D12_HEAP_PROPERTIES textureHeapProps{};
	textureHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	textureHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	textureHeapProps.CreationNodeMask = 1;
	textureHeapProps.VisibleNodeMask = 1;
	D3D12_RESOURCE_DESC textureDesc{};
	textureDesc.MipLevels = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	textureDesc.Width = textureWidth;
	textureDesc.Height = textureHeight;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	result = _device->CreateCommittedResource(&textureHeapProps, D3D12_HEAP_FLAG_NONE, &textureDesc,
		D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&_texture));
	if (result != S_OK) {
		std::cout << "Failed to map texture buffer! : " << result << std::endl;
	}
	
	ResourceUploader textureUploader;
	if (stbimg != nullptr) {
		textureUploader.updateSubresource(commandList, 0, stbimg, textureWidth * textureHeight * 4, _texture.Get());
		stbi_image_free(stbimg);
	}
	else {
		textureUploader.updateSubresource(commandList, 0, checkboxTexturePointer, textureWidth* textureHeight * 4, _texture.Get());
	}

	// texture SRV heap
	D3D12_DESCRIPTOR_HEAP_DESC textureSRVHeapDesc{};
	textureSRVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	textureSRVHeapDesc.NodeMask = 0;
	textureSRVHeapDesc.NumDescriptors = 1;
	textureSRVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	result = _device->CreateDescriptorHeap(&textureSRVHeapDesc, IID_PPV_ARGS(&_textureSRVHeap));

	D3D12_SHADER_RESOURCE_VIEW_DESC textureSRVDesc{};
	textureSRVDesc.Format = textureDesc.Format;
	textureSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	textureSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	textureSRVDesc.Texture2D.MipLevels = 1;
	_device->CreateShaderResourceView(_texture.Get(), &textureSRVDesc, _textureSRVHeap->GetCPUDescriptorHandleForHeapStart());
	
	result = commandList->Close();
	ID3D12CommandList* commandLists[] = { commandList };
	_queue->ExecuteCommandLists(1, commandLists);
	_waitForGpu();
}

void SimpleRenderer::_initRootSignature() {
	HRESULT result = S_OK;

	// Signature
	D3D12_DESCRIPTOR_RANGE signatureSRVDescriptorTableRange{};
	signatureSRVDescriptorTableRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	signatureSRVDescriptorTableRange.BaseShaderRegister = 0;
	signatureSRVDescriptorTableRange.RegisterSpace = 0;
	signatureSRVDescriptorTableRange.NumDescriptors = 1;
	signatureSRVDescriptorTableRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER signatureParams[3]{};
	signatureParams[0].Descriptor.RegisterSpace = 0;
	signatureParams[0].Descriptor.ShaderRegister = 0;
	signatureParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	signatureParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	signatureParams[1].Descriptor.RegisterSpace = 0;
	signatureParams[1].Descriptor.ShaderRegister = 1;
	signatureParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	signatureParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	signatureParams[2].DescriptorTable.NumDescriptorRanges = 1;
	signatureParams[2].DescriptorTable.pDescriptorRanges = &signatureSRVDescriptorTableRange;
	signatureParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	signatureParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_STATIC_SAMPLER_DESC staticSamplers[1]{};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = staticSamplers[0].AddressV = staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].MaxAnisotropy = 16;

	D3D12_ROOT_SIGNATURE_DESC signatureDesc = {};
	signatureDesc.NumParameters = _countof(signatureParams);
	signatureDesc.pParameters = signatureParams;
	signatureDesc.NumStaticSamplers = 1;
	signatureDesc.pStaticSamplers = staticSamplers;
	signatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	result = D3D12SerializeRootSignature(&signatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
	if (result < 0) {
		std::cerr << "Failed to serialize root signature! " << result << std::endl;
	}

	result = _device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&_rootSignature));
	if (result < 0) {
		std::cerr << "Failed to create root signature! " << result << std::endl;
	}
}

void SimpleRenderer::_cleanupAssets() {
	_commonBuffer->close();
	_uniformBuffer->close();

	// Because we use ComPtr reference, there's no need to release assets explicitly.
}

void SimpleRenderer::update(float deltaTime) {
	CommonInfo commonInfo = {};
	commonInfo.normalizedSDRWhiteLevel = _referenceSDRWhiteNits / 10000.0f;
	commonInfo.isST2084Output = _isHDROutputSupported;
	_commonBuffer->copy(&commonInfo, sizeof(CommonInfo), _currentFrameIndex * sizeof(CommonInfo));

	float aspectRatio = _width / (float)_height;
	ObjectInfo objInfo = {};
	objInfo.view = XMMatrixTranspose(XMMatrixRotationZ(Time::getTimeSinceStartup()));
	objInfo.projection = XMMatrixTranspose(XMMatrixOrthographicLH(2.0f * aspectRatio, 2.0f, -1.0f, 1.0f));
	_uniformBuffer->copy(&objInfo, sizeof(ObjectInfo), _currentFrameIndex * sizeof(ObjectInfo));
}

void SimpleRenderer::render() {
	auto commandList = _getRenderCommandList();
	commandList->SetName(L"Draw");
	commandList->SetGraphicsRootSignature(_rootSignature.Get());

	PIXBeginEvent(commandList, 0, "Draw");
	commandList->SetPipelineState(_renderPipeline.Get());
	commandList->IASetVertexBuffers(0, 1, &_vertexBufferView);

	ID3D12DescriptorHeap* descriptorHeaps[] = { _textureSRVHeap.Get() };
	commandList->SetDescriptorHeaps(1, descriptorHeaps);
	auto commonBufferAddress = _commonBuffer->getResource()->GetGPUVirtualAddress();
	commonBufferAddress += (UINT64)_currentFrameIndex * sizeof(CommonInfo);
	commandList->SetGraphicsRootConstantBufferView(0, commonBufferAddress);
	auto uniformBufferAddress = _uniformBuffer->getResource()->GetGPUVirtualAddress();
	uniformBufferAddress += (UINT64)_currentFrameIndex * sizeof(ObjectInfo);
	commandList->SetGraphicsRootConstantBufferView(1, uniformBufferAddress);
	commandList->SetGraphicsRootDescriptorTable(2, _textureSRVHeap->GetGPUDescriptorHandleForHeapStart());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawInstanced(_countof(kVertices), 1, 0, 0);

	PIXEndEvent(commandList);
}