#include "SimpleRenderer.h"
#include "../Common/Time.h"
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <string>
#include <iostream>
#include <functional>

using namespace DirectX;

struct VertexInfo {
	XMFLOAT3 pos;
	XMFLOAT3 color;
};

static VertexInfo kVertices[] = {
	{ XMFLOAT3(-0.5f, 0.0f, -0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
	{ XMFLOAT3(-0.5f, 0.0f, 0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
	{ XMFLOAT3(0.5f, 0.0f, -0.5f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
	{ XMFLOAT3(0.5f, 0.0f, -0.5f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
	{ XMFLOAT3(-0.5f, 0.0f, 0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
	{ XMFLOAT3(0.5f, 0.0f, 0.5f), XMFLOAT3(1.0f, 1.0f, 1.0f) }
};
 
_declspec(align(256)) struct ObjectInfo {
	XMMATRIX model;
	XMMATRIX view;
	XMMATRIX projection;
	float time;
};

constexpr UINT textureWidth = 32;

void SimpleRenderer::init() {
	_initAssets();
}

void SimpleRenderer::_initAssets() {
	HRESULT result = S_OK;

	// Shader
	ComPtr<ID3DBlob> vertexShader, pixelShader, hullShader, domainShader;
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

	std::wstring vertexShaderPath(rootPath), pixelShaderPath(rootPath), hullShaderPath(rootPath), domainShaderPath(rootPath);
	vertexShaderPath += L"VertexShader.cso";
	pixelShaderPath += L"PixelShader.cso";
	hullShaderPath += L"HullShader.cso";
	domainShaderPath += L"DomainShader.cso";

	result = D3DReadFileToBlob(vertexShaderPath.c_str(), &vertexShader);
	if (result != S_OK) {
		std::wcout << L"Failed to load shader at " << vertexShaderPath.c_str() << std::endl;
		return;
	}
	_device->CreateVertexShader(vertexShader->GetBufferPointer(), vertexShader->GetBufferSize(), nullptr, &_vertexShader);

	result = D3DReadFileToBlob(pixelShaderPath.c_str(), &pixelShader);
	if (result != S_OK) {
		std::wcout << L"Failed to load shader at " << pixelShaderPath.c_str() << std::endl;
		return;
	}
	_device->CreatePixelShader(pixelShader->GetBufferPointer(), pixelShader->GetBufferSize(), nullptr, &_pixelShader);

	result = D3DReadFileToBlob(hullShaderPath.c_str(), &hullShader);
	if (result != S_OK) {
		std::wcout << L"Failed to load shader at " << hullShaderPath.c_str() << std::endl;
		return;
	}
	_device->CreateHullShader(hullShader->GetBufferPointer(), hullShader->GetBufferSize(), nullptr, &_hullShader);

	result = D3DReadFileToBlob(domainShaderPath.c_str(), &domainShader);
	if (result != S_OK) {
		std::wcout << L"Failed to load shader at " << domainShaderPath.c_str() << std::endl;
		return;
	}
	_device->CreateDomainShader(domainShader->GetBufferPointer(), domainShader->GetBufferSize(), nullptr, &_domainShader);


	// Input element
	D3D11_INPUT_ELEMENT_DESC inputElementDescs[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(XMFLOAT3), D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	result = _device->CreateInputLayout(inputElementDescs, _countof(inputElementDescs), vertexShader->GetBufferPointer(), vertexShader->GetBufferSize(), &_inputLayout);
	if (result != S_OK) {
		std::cerr << "Failed to create input layout!" << std::endl;
		return;
	}

	// Vertex buffer
	D3D11_BUFFER_DESC vertexBufferDesc = {};
	vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vertexBufferDesc.ByteWidth = sizeof(kVertices);
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	D3D11_SUBRESOURCE_DATA vertexResourceData = {};
	vertexResourceData.pSysMem = kVertices;
	result = _device->CreateBuffer(&vertexBufferDesc, &vertexResourceData, &_vertexBuffer);
	if (result != S_OK) {
		std::cerr << "Failed to create vertex buffer!" << std::endl;
		return;
	}

	// Uniform buffer
	D3D11_BUFFER_DESC uniformBufferDesc = {};
	uniformBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	uniformBufferDesc.ByteWidth = sizeof(ObjectInfo);
	uniformBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	uniformBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	result = _device->CreateBuffer(&uniformBufferDesc, nullptr, &_uniformBuffer);
	if (result != S_OK) {
		std::cerr << "Failed to create uniform buffer!" << std::endl;
		return;
	}

	// Rasterizer state
	D3D11_RASTERIZER_DESC rasterizerDesc = {};
	rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;
	rasterizerDesc.AntialiasedLineEnable = true;
	result = _device->CreateRasterizerState(&rasterizerDesc, &_rasterizerState);
	if (result != S_OK) {
		std::cerr << "Failed to create rasterizer state!" << std::endl;
		return;
	}

	// texture
	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	textureDesc.Width = textureWidth;
	textureDesc.Height = textureWidth;
	textureDesc.ArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Format = DXGI_FORMAT_R8_UNORM;
	textureDesc.MipLevels = 1;
	constexpr UINT textureSize = textureWidth * textureWidth;
	UINT8 texturePointer[textureSize] = { };
	_initNoiseTexture(texturePointer);
	D3D11_SUBRESOURCE_DATA textureResourceData = {};
	textureResourceData.pSysMem = texturePointer;
	textureResourceData.SysMemPitch = textureWidth;
	result = _device->CreateTexture2D(&textureDesc, &textureResourceData, &_texture);
	if (result != S_OK) {
		std::cerr << "Failed to create texture 2D!" << std::endl;
		return;
	}

	// shader resource view
	D3D11_SHADER_RESOURCE_VIEW_DESC textureSRVDesc = {};
	textureSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	textureSRVDesc.Texture2D.MipLevels = 1;
	result = _device->CreateShaderResourceView(_texture.Get(), &textureSRVDesc, &_textureSRV);
	if (result != S_OK) {
		std::cerr << "Failed to create shader resource view for texture!" << std::endl;
		return;
	}

	// sampler
	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	result = _device->CreateSamplerState(&samplerDesc, &_sampler);
	if (result != S_OK) {
		std::cerr << "Failed to create sampler state!" << std::endl;
		return;
	}
}

void SimpleRenderer::_initNoiseTexture(UINT8 *texturePointer) {
	// make simple noise textures!
	UINT8 noiseDiv4[textureWidth * textureWidth / 16] = {};
	UINT8 noiseDiv2[textureWidth * textureWidth / 4] = {};
	UINT8 noise[textureWidth * textureWidth] = {};

	for (int j = 0; j < textureWidth; j++) {
		for (int i = 0; i < textureWidth; i++) {
			if (j % 4 == 0 && i % 4 == 0)
				noiseDiv4[j * textureWidth / 16 + i / 4] = static_cast<UINT8>((float)rand() / RAND_MAX * 255.0f);
			if (j % 2 == 0 && i % 2 == 0)
				noiseDiv2[j * textureWidth / 4 + i / 2] = static_cast<UINT8>((float)rand() / RAND_MAX * 255.0f);
			noise[j * textureWidth + i] = static_cast<UINT8>((float)rand() / RAND_MAX * 255.0f);
		}
	}

	auto lerp = [](UINT a, UINT b, float t) { return static_cast<UINT8>(a * (1.0f - t) + b * t); };

	for (int j = 0; j < textureWidth; j++) {
		for (int i = 0; i < textureWidth; i++) {
			texturePointer[j * textureWidth + i] = static_cast<UINT8>(noise[j * textureWidth + i] * 0.125f);
			UINT8 l1 = lerp(noiseDiv2[j * textureWidth / 4 + i / 2], noiseDiv2[j * textureWidth / 4 + i / 2 + 1], (i % 2) / 2.0f);
			UINT8 l2 = lerp(noiseDiv2[(j + 1) * textureWidth / 4 + i / 2], noiseDiv2[(j + 1) * textureWidth / 4 + i / 2 + 1], (i % 2) / 2.0f);
			texturePointer[j * textureWidth + i] += static_cast<UINT8>(lerp(l1, l2, (j % 2) / 2.0f) * 0.25f);
			l1 = lerp(noiseDiv4[j * textureWidth / 16 + i / 4], noiseDiv4[j * textureWidth / 16 + i / 4 + 1], (i % 4) / 4.0f);
			l2 = lerp(noiseDiv4[(j + 1) * textureWidth / 16 + i / 4], noiseDiv4[(j + 1) * textureWidth / 16 + i / 4 + 1], (i % 4) / 4.0f);
			texturePointer[j * textureWidth + i] += static_cast<UINT8>(lerp(l1, l2, (j % 4) / 4.0f) * 0.5f);
		}
	}
}

void SimpleRenderer::update(float deltaTime) {
	float aspectRatio = _width / (float)_height;
	ObjectInfo info = {};

	info.model = XMMatrixTranspose(XMMatrixRotationRollPitchYaw(0.0f, Time::getTimeSinceStartup(), 0.0f));
	info.view = XMMatrixTranspose(XMMatrixLookAtLH(XMVectorSet(0.0f, 0.4f, -1.0f, 0.0f), XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));
	info.projection = XMMatrixTranspose(XMMatrixPerspectiveFovLH(60.0f / 180.0f * 3.14159, aspectRatio, 0.25f, 1000.0f));
	info.time = std::fmin(Time::getTimeSinceStartup(), 100.0f);
	D3D11_MAPPED_SUBRESOURCE uniformSubresource = {};
	_context->Map(_uniformBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &uniformSubresource);
	memcpy(uniformSubresource.pData, &info, sizeof(ObjectInfo));
	_context->Unmap(_uniformBuffer.Get(), 0);
}

void SimpleRenderer::render() {
	UINT stride = sizeof(VertexInfo);
	UINT offset = 0;
	_context->IASetInputLayout(_inputLayout.Get());
	_context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
	_context->IASetVertexBuffers(0, 1, _vertexBuffer.GetAddressOf(), &stride, &offset);
	_context->VSSetShader(_vertexShader.Get(), nullptr, 0);
	_context->VSSetConstantBuffers(0, 1, _uniformBuffer.GetAddressOf());
	_context->RSSetState(_rasterizerState.Get());
	_context->PSSetShader(_pixelShader.Get(), nullptr, 0);
	_context->PSSetConstantBuffers(0, 1, _uniformBuffer.GetAddressOf());
	_context->PSSetShaderResources(0, 1, _textureSRV.GetAddressOf());
	_context->PSSetSamplers(0, 1, _sampler.GetAddressOf());
	_context->HSSetShader(_hullShader.Get(), nullptr, 0);
	_context->HSSetConstantBuffers(0, 1, _uniformBuffer.GetAddressOf());
	_context->DSSetShader(_domainShader.Get(), nullptr, 0);
	_context->DSSetShaderResources(0, 1, _textureSRV.GetAddressOf());
	_context->DSSetSamplers(0, 1, _sampler.GetAddressOf());
	_context->DrawInstanced(_countof(kVertices), 3, 0, 0);
}