#include <stdio.h>
#include <iostream>
#include <vector>


#include "Blur.h"


Blur::~Blur(){}

Blur::Blur(int width, int height, int depth){
	_passthrough.init("passthrough");
	// Create a series of framebuffers smaller and smaller.
	_frameBuffers = std::vector<std::shared_ptr<Framebuffer>>(depth);
	for(size_t i = 0; i < depth; ++i){
		_frameBuffers[i] = std::make_shared<Framebuffer>(width/std::pow(2,i), height/std::pow(2,i), GL_RGB, GL_FLOAT, GL_RGB, GL_LINEAR, GL_CLAMP_TO_EDGE);
	}
	checkGLError();
}


void Blur::process(const GLuint textureId) {
	if(_frameBuffers.size() == 0){
		return;
	}
	
	glClearColor(0.0f,0.0f,0.0f,0.0f);
	// First, copy the input texture to the first framebuffer.
	_frameBuffers[0]->bind();
	glViewport(0, 0, _frameBuffers[0]->width(), _frameBuffers[0]->height());
	glClear(GL_COLOR_BUFFER_BIT);
	_passthrough.draw(textureId);
	_frameBuffers[0]->unbind();
	
	// Then iterate over all framebuffers, cascading down the texture.
	for(size_t i = 1; i < _frameBuffers.size(); ++i){
		_frameBuffers[i]->bind();
		glViewport(0, 0, _frameBuffers[i]->width(), _frameBuffers[i]->height());
		glClear(GL_COLOR_BUFFER_BIT);
		_passthrough.draw(_frameBuffers[i-1]->textureId());
		_frameBuffers[i]->unbind();
	}
	
}

void Blur::draw() {
	if(_frameBuffers.size() == 0){
		return;
	}
	
	_passthrough.draw(_frameBuffers.back()->textureId());
}


void Blur::clean() const {
	for(auto & frameBuffer : _frameBuffers){
		frameBuffer->clean();
	}
	_passthrough.clean();
}


void Blur::resize(int width, int height){
	
}

