#include "DeferredRenderer.h"

void DeferredRenderer::init() {
	_initAssets();
}

void DeferredRenderer::_initAssets() {
	_gBuffer = std::make_unique<GBuffer>(_device, _width, _height);
}