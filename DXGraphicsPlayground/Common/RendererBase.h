#pragma once

// Renderer base class
class RendererBase
{
public:
	virtual void update(float deltaTime) = 0;
	virtual void render() = 0;
	virtual void resize(int newWidth, int newHeight) = 0;

	virtual void beginFrame() = 0;
	virtual void endFrame() = 0;
};

