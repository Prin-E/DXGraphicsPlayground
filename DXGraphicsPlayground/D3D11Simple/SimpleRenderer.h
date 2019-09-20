#pragma once

#include "../Common/RendererD3D11.h"
#include <memory>

class SimpleRenderer : public RendererD3D11
{
public:
	virtual void update(float deltaTime) override;
	virtual void init() override;
	virtual void render() override;
	//virtual void resize(int newWidth, int newHeight) override;

protected:
	void _initAssets();
	void _cleanupAssets();
	void _initNoiseTexture(UINT8 *texturePointer);

private:
	ComPtr<ID3D11InputLayout> _inputLayout;
	ComPtr<ID3D11Buffer> _vertexBuffer;
	ComPtr<ID3D11Buffer> _uniformBuffer;
	ComPtr<ID3D11Texture2D> _texture;
	ComPtr<ID3D11ShaderResourceView> _textureSRV;
	ComPtr<ID3D11SamplerState> _sampler;
	ComPtr<ID3D11VertexShader> _vertexShader;
	ComPtr<ID3D11HullShader> _hullShader;
	ComPtr<ID3D11DomainShader> _domainShader;
	ComPtr<ID3D11PixelShader> _pixelShader;
	ComPtr<ID3D11RasterizerState> _rasterizerState;
};