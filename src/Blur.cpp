#include <stdio.h>
#include <iostream>
#include <vector>


#include "Blur.h"


Blur::~Blur(){}

Blur::Blur(int width, int height, int depth){
	_passthrough.init("passthrough");
	_frameBuffer0 = std::make_shared<Framebuffer>(width, height, GL_RGB, GL_FLOAT, GL_RGB, GL_LINEAR, GL_CLAMP_TO_EDGE);
	checkGLError();
}


void Blur::process(const GLuint textureId) {
	_frameBuffer0->bind();
	glViewport(0, 0, _frameBuffer0->width(), _frameBuffer0->height());
	_passthrough.draw(textureId, glm::vec2(0.0f,0.0f));
	_frameBuffer0->unbind();
}

void Blur::draw() {
	_passthrough.draw(_frameBuffer0->textureId(), glm::vec2(0.0f,0.0f));
}


void Blur::clean() const {
	_frameBuffer0->clean();
	_passthrough.clean();
}


void Blur::resize(int width, int height){
	
}

