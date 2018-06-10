#include "Blur.hpp"

#include <stdio.h>
#include <vector>

Blur::~Blur(){}

Blur::Blur(){
	_passthrough.init("passthrough");
}

void Blur::process(const GLuint textureId) {

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


void Blur::resize(int width, int height){
}

