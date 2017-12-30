#include <stdio.h>
#include <iostream>
#include <vector>


#include "Blur.hpp"


Blur::~Blur(){}

Blur::Blur(int width, int height, int depth){
	_passthrough.init("passthrough");
	_blurScreen.init("blur");
	
	// Create a series of framebuffers smaller and smaller.
	_frameBuffers = std::vector<std::shared_ptr<Framebuffer>>(depth);
	_frameBuffersBlur = std::vector<std::shared_ptr<Framebuffer>>(depth);
	std::map<std::string, GLuint> textures;
	for(size_t i = 0; i < depth; ++i){
		_frameBuffers[i] = std::make_shared<Framebuffer>((int)(width/std::pow(2,i)), (int)(height/std::pow(2,i)), GL_RGB, GL_FLOAT, GL_RGB16F, GL_LINEAR, GL_CLAMP_TO_EDGE, false);
		_frameBuffersBlur[i] = std::make_shared<Framebuffer>((int)(width/std::pow(2,i)), (int)(height/std::pow(2,i)), GL_RGB, GL_FLOAT, GL_RGB16F, GL_LINEAR, GL_CLAMP_TO_EDGE, false);
		textures["texture" + std::to_string(i)] = _frameBuffers[i]->textureId();
	}

	// Final combining buffer.
	if (_frameBuffers.size() > 1) {
		_combineScreen.init(textures, "blur-combine-" + std::to_string(_frameBuffers.size()));
	}
	_finalFramebuffer = std::make_shared<Framebuffer>(width, height, GL_RGB, GL_FLOAT, GL_RGB16F, GL_LINEAR, GL_CLAMP_TO_EDGE, false);
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
	
	if (_frameBuffers.size() == 1) {
		// No need to merge.
		return;
	}

	_finalFramebuffer->bind();
	glViewport(0, 0, _finalFramebuffer->width(), _finalFramebuffer->height());
	glClear(GL_COLOR_BUFFER_BIT);
	_combineScreen.draw();
	_finalFramebuffer->unbind();

}

void Blur::draw() {
	if(_frameBuffers.size() == 0){
		return;
	} else if (_frameBuffers.size() == 1) {
		_passthrough.draw(_frameBuffers.front()->textureId());
	} else {
		_passthrough.draw(_finalFramebuffer->textureId());
	}
	
}


void Blur::clean() const {
	for(auto & frameBuffer : _frameBuffers){
		frameBuffer->clean();
	}
	for (auto & frameBuffer : _frameBuffersBlur) {
		frameBuffer->clean();
	}
	_passthrough.clean();
	_blurScreen.clean();
	_combineScreen.clean();
}


void Blur::resize(int width, int height){
	// Unsupported for now.
}

