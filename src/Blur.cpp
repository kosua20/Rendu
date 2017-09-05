#include <stdio.h>
#include <iostream>
#include <vector>


#include "Blur.h"


Blur::~Blur(){}

Blur::Blur(int width, int height, int depth){
	_passthrough.init("passthrough");
	_blurScreen.init("blur");
	// Create a series of framebuffers smaller and smaller.
	_frameBuffers = std::vector<std::shared_ptr<Framebuffer>>(depth);
	_frameBuffersBlur = std::vector<std::shared_ptr<Framebuffer>>(depth);
	for(size_t i = 0; i < depth; ++i){
		_frameBuffers[i] = std::make_shared<Framebuffer>(width/std::pow(2,i), height/std::pow(2,i), GL_RGB, GL_FLOAT, GL_RGB, GL_LINEAR, GL_CLAMP_TO_EDGE);
		_frameBuffersBlur[i] = std::make_shared<Framebuffer>(width/std::pow(2,i), height/std::pow(2,i), GL_RGB, GL_FLOAT, GL_RGB, GL_LINEAR, GL_CLAMP_TO_EDGE);
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
	
	// Blur vertically each framebuffer into frameBuffersBlur.
	for(size_t i = 0; i < _frameBuffers.size(); ++i){
		_frameBuffersBlur[i]->bind();
		glViewport(0, 0, _frameBuffersBlur[i]->width(), _frameBuffersBlur[i]->height());
		glClear(GL_COLOR_BUFFER_BIT);
		const glm::vec2 invResolution(0.0f, 1.2f/(float)_frameBuffersBlur[i]->height());
		_blurScreen.draw(_frameBuffers[i]->textureId(), invResolution);
		_frameBuffersBlur[i]->unbind();
	}
	// Blur horiontally each framebufferBlur back into frameBuffers.
	for(size_t i = 0; i < _frameBuffersBlur.size(); ++i){
		_frameBuffers[i]->bind();
		glViewport(0, 0, _frameBuffers[i]->width(), _frameBuffers[i]->height());
		glClear(GL_COLOR_BUFFER_BIT);
		const glm::vec2 invResolution(1.2f/(float)_frameBuffers[i]->width(), 0.0f);
		_blurScreen.draw(_frameBuffersBlur[i]->textureId(), invResolution);
		_frameBuffers[i]->unbind();
	}
	
	_frameBuffers[0]->bind();
	glEnable(GL_BLEND);
	glViewport(0, 0, _frameBuffers[0]->width(), _frameBuffers[0]->height());
	for(size_t i = 1; i < _frameBuffers.size(); ++i){
		_passthrough.draw(_frameBuffers[i]->textureId());
	}
	glDisable(GL_BLEND);
	_frameBuffers[0]->unbind();
}

void Blur::draw() {
	if(_frameBuffers.size() == 0){
		return;
	}
	_passthrough.draw(_frameBuffers.front()->textureId());
	
}


void Blur::clean() const {
	for(auto & frameBuffer : _frameBuffers){
		frameBuffer->clean();
	}
	_passthrough.clean();
}


void Blur::resize(int width, int height){
	
}

