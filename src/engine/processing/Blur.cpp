#include "Blur.hpp"

Blur::~Blur(){}

Blur::Blur(){
	_passthroughProgram = Resources::manager().getProgram("passthrough");
	_passthroughProgram->registerTexture("screenTexture", 0);
}

void Blur::process(const GLuint ) {

}

void Blur::draw() {
	glUseProgram(_passthroughProgram->id());
	ScreenQuad::draw(_finalTexture);
}

GLuint Blur::textureId() const {
	return _finalTexture;
}

void Blur::clean() const {
}


void Blur::resize(unsigned int , unsigned int ){
}

