#include "Renderer.hpp"

Renderer::Renderer(const std::string & name) : _name(name) {

}

void Renderer::draw(const Camera &, Texture*, Texture*, uint){
	Log::Error() << "Renderer: Unimplemented draw function." << std::endl;
	assert(false);
}

void Renderer::interface(){
	Log::Error() << "Renderer: Unimplemented interface function." << std::endl;
	assert(false);
}

void Renderer::resize(uint, uint){
}
