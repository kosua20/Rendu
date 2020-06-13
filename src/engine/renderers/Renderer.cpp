#include "Renderer.hpp"


void Renderer::draw(const Camera &, Framebuffer &, size_t){
	Log::Error() << "Renderer: Unimplemented draw function." << std::endl;
	assert(false);
}

void Renderer::interface(){
	Log::Error() << "Renderer: Unimplemented interface function." << std::endl;
	assert(false);
}

void Renderer::resize(unsigned int, unsigned int){
}

std::unique_ptr<Framebuffer> Renderer::createOutput(uint width, uint height) const {
	return createOutput(TextureShape::D2, width, height, 1, 1);
}

std::unique_ptr<Framebuffer> Renderer::createOutput(TextureShape shape, uint width, uint height, uint depth, uint mips) const {
	return std::unique_ptr<Framebuffer>(new Framebuffer(shape, width, height, depth, mips, _preferredFormat, _needsDepth));
}

