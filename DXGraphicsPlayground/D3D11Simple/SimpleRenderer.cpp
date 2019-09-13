#include "SimpleRenderer.h"
#include "../Common/Time.h"
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <string>
#include <iostream>

using namespace DirectX;

struct VertexInfo {
	XMFLOAT3 pos;
	XMFLOAT3 color;
};

static VertexInfo kVertices[] = {
	{ XMFLOAT3(-0.5f, -0.3f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
	{ XMFLOAT3(0.0f,  0.6f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
	{ XMFLOAT3(0.5f, -0.3f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
};

_declspec(align(256)) struct ObjectInfo {
	XMMATRIX rotation;
	XMMATRIX projection;
};

void SimpleRenderer::init() {
	_initAssets();
}

void SimpleRenderer::_initAssets() {
	HRESULT result = S_OK;

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
	_device->CreateVertexShader(vertexShader->GetBufferPointer(), vertexShader->GetBufferSize(), nullptr, &_vertexShader);

	result = D3DReadFileToBlob(pixelShaderPath.c_str(), &pixelShader);
	if (result != S_OK) {
		std::wcout << L"Failed to load shader at " << pixelShaderPath.c_str() << std::endl;
		return;
	}
	_device->CreatePixelShader(pixelShader->GetBufferPointer(), pixelShader->GetBufferSize(), nullptr, &_pixelShader);

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

	// texture
	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	textureDesc.Width = 32;
	textureDesc.Height = 32;
	textureDesc.ArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.MipLevels = 1;
	const size_t textureSize = textureDesc.Width * textureDesc.Height * 4;
	UINT8 *texturePointer = new UINT8[textureSize];
	for (int j = 0; j < textureDesc.Height; j++) {
		for (int i = 0; i < textureDesc.Width; i++) {
			texturePointer[(j * textureDesc.Width + i) * 4] = (i + j) % 2 ? 255 : 0;
			texturePointer[(j * textureDesc.Width + i) * 4 + 1] = (i + j) % 2 ? 255 : 0;
			texturePointer[(j * textureDesc.Width + i) * 4 + 2] = (i + j) % 2 ? 255 : 0;
			texturePointer[(j * textureDesc.Width + i) * 4 + 3] = 255;
		}
	}

	D3D11_SUBRESOURCE_DATA textureResourceData = {};
	textureResourceData.pSysMem = texturePointer;
	textureResourceData.SysMemPitch = textureDesc.Width * 4;
	result = _device->CreateTexture2D(&textureDesc, &textureResourceData, &_texture);
	if (result != S_OK) {
		std::cerr << "Failed to create texture 2D!" << std::endl;
		return;
	}
	delete texturePointer;

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
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	result = _device->CreateSamplerState(&samplerDesc, &_sampler);
	if (result != S_OK) {
		std::cerr << "Failed to create sampler state!" << std::endl;
		return;
	}
}

void SimpleRenderer::update(float deltaTime) {
	float aspectRatio = _width / (float)_height;
	ObjectInfo info = {};
	info.rotation = XMMatrixTranspose(XMMatrixRotationZ(Time::getTimeSinceStartup()));
	info.projection = XMMatrixTranspose(XMMatrixOrthographicLH(2.0f * aspectRatio, 2.0f, -1.0f, 1.0f));
	D3D11_MAPPED_SUBRESOURCE uniformSubresource = {};
	_context->Map(_uniformBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &uniformSubresource);
	memcpy(uniformSubresource.pData, &info, sizeof(ObjectInfo));
	_context->Unmap(_uniformBuffer.Get(), 0);
}

void SimpleRenderer::render() {
	UINT stride = sizeof(VertexInfo);
	UINT offset = 0;
	_context->IASetInputLayout(_inputLayout.Get());
	_context->IASetVertexBuffers(0, 1, _vertexBuffer.GetAddressOf(), &stride, &offset);
	_context->VSSetShader(_vertexShader.Get(), nullptr, 0);
	_context->VSSetConstantBuffers(0, 1, _uniformBuffer.GetAddressOf());
	_context->PSSetShader(_pixelShader.Get(), nullptr, 0);
	_context->PSSetConstantBuffers(0, 1, _uniformBuffer.GetAddressOf());
	_context->PSSetShaderResources(0, 1, _textureSRV.GetAddressOf());
	_context->PSSetSamplers(0, 1, _sampler.GetAddressOf());
	_context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_context->DrawInstanced(_countof(kVertices), 1, 0, 0);
}