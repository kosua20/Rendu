#include "Renderer.hpp"

Renderer::Renderer(const std::string & name) : _name(name) {

}

void Renderer::draw(const Camera &, Framebuffer &, uint){
	Log::Error() << "Renderer: Unimplemented draw function." << std::endl;
	assert(false);
}

void Renderer::interface(){
	Log::Error() << "Renderer: Unimplemented interface function." << std::endl;
	assert(false);
}

void Renderer::resize(uint, uint){
}

std::unique_ptr<Framebuffer> Renderer::createOutput(uint width, uint height, const std::string & name) const {
	return createOutput(TextureShape::D2, width, height, 1, 1, name);
}

std::unique_ptr<Framebuffer> Renderer::createOutput(TextureShape shape, uint width, uint height, uint depth, uint mips, const std::string & name) const {
	return std::unique_ptr<Framebuffer>(new Framebuffer(shape, width, height, depth, mips, _preferredFormat, name));
}

