#include "GaussianBlur.hpp"


GaussianBlur::GaussianBlur(unsigned int width, unsigned int height, unsigned int depth, GLuint preciseFormat) : Blur() {
	_passthroughProgram = Resources::manager().getProgram("passthrough");
	_blurProgramDown = Resources::manager().getProgram2D("blur-dual-filter-down");
	_blurProgramUp = Resources::manager().getProgram2D("blur-dual-filter-up");
	
	// Create a series of framebuffers smaller and smaller.
	_frameBuffers = std::vector<std::unique_ptr<Framebuffer>>(depth);
	
	for(size_t i = 0; i < (size_t)depth; ++i){
		_frameBuffers[i] = std::unique_ptr<Framebuffer>(new Framebuffer((unsigned int)(width/std::pow(2,i)), (unsigned int)(height/std::pow(2,i)), preciseFormat , false));
	}

	_finalTexture = _frameBuffers[0]->textureId();
	checkGLError();
}

void GaussianBlur::process(const GLuint textureId) {
	if(_frameBuffers.size() == 0){
		return;
	}
	
	glClearColor(0.0f,0.0f,0.0f,0.0f);
	// First, copy the input texture to the first framebuffer.
	_frameBuffers[0]->bind();
	_frameBuffers[0]->setViewport();
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(_passthroughProgram->id());
	ScreenQuad::draw(textureId);
	_frameBuffers[0]->unbind();
	
	// Downscale filter.
	glUseProgram(_blurProgramDown->id());
	for(int d = 1; d < _frameBuffers.size(); ++d){
		_frameBuffers[d]->bind();
		_frameBuffers[d]->setViewport();
		glClear(GL_COLOR_BUFFER_BIT);
		
		ScreenQuad::draw(_frameBuffers[d-1]->textureId());
		_frameBuffers[d]->unbind();
	}
	
	// Upscale filter.
	glUseProgram(_blurProgramUp->id());
	for(int d = _frameBuffers.size()-2; d >= 0; --d){
		_frameBuffers[d]->bind();
		_frameBuffers[d]->setViewport();
		glClear(GL_COLOR_BUFFER_BIT);
		ScreenQuad::draw(_frameBuffers[d+1]->textureId());
		_frameBuffers[d]->unbind();
	}
	
}


void GaussianBlur::clean() const {
	for(auto & frameBuffer : _frameBuffers){
		frameBuffer->clean();
	}
	Blur::clean();
}


void GaussianBlur::resize(unsigned int width, unsigned int height){
	for(size_t i = 0; i < _frameBuffers.size(); ++i){
		const unsigned int hwidth = (unsigned int)(width/std::pow(2,i));
		const unsigned int hheight = (unsigned int)(height/std::pow(2,i));
		_frameBuffers[i]->resize(hwidth, hheight);
	}
}

