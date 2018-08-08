#include "Blur.hpp"

Blur::~Blur(){}

Blur::Blur(){
	_passthrough.init("passthrough");
}

void Blur::process(const GLuint ) {

}

void Blur::draw() {
	_passthrough.draw(_finalTexture);
}

GLuint Blur::textureId() const {
	return _finalTexture;
}

void Blur::clean() const {
	_passthrough.clean();
}


void Blur::resize(unsigned int , unsigned int ){
}

