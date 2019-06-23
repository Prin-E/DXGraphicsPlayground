#pragma once

#include "../Common/GBuffer.h"
#include "../Common/GPUBuffer.h"
#include "../Common/RendererD3D12.h"
#include "../Common/Time.h"
#include <memory>

class DeferredRenderer : public RendererD3D12
{
public:
	virtual void init() override;

	// Rendering
	virtual void update(float deltaTime) override;
	virtual void render() override;

protected:
	void _initAssets();

private:
	std::unique_ptr<GBuffer> _gBuffer;
};

