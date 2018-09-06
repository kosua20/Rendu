#include "GaussianBlur.hpp"


GaussianBlur::GaussianBlur(unsigned int width, unsigned int height, unsigned int depth, GLuint format, GLuint type, GLuint preciseFormat) : Blur() {
	_passthroughProgram = Resources::manager().getProgram("passthrough");
	_blurProgram = Resources::manager().getProgram2D("blur");
	
	// Create a series of framebuffers smaller and smaller.
	_frameBuffers = std::vector<std::shared_ptr<Framebuffer>>(depth);
	_frameBuffersBlur = std::vector<std::shared_ptr<Framebuffer>>(depth);
	
	_textures.resize(depth);
	
	for(size_t i = 0; i < (size_t)depth; ++i){
		_frameBuffers[i] = std::make_shared<Framebuffer>((unsigned int)(width/std::pow(2,i)), (unsigned int)(height/std::pow(2,i)), format, type, preciseFormat, GL_LINEAR, GL_CLAMP_TO_EDGE, false);
		_frameBuffersBlur[i] = std::make_shared<Framebuffer>((unsigned int)(width/std::pow(2,i)), (unsigned int)(height/std::pow(2,i)), format, type, preciseFormat, GL_LINEAR, GL_CLAMP_TO_EDGE, false);
		_textures[i] = _frameBuffers[i]->textureId();
	}

	// Final combining buffer.
	if (_frameBuffers.size() > 1) {
		const std::string combineProgramName = "blur-combine-" + std::to_string(_frameBuffers.size());
		_combineProgram = Resources::manager().getProgram2D(combineProgramName);
		_finalFramebuffer = std::make_shared<Framebuffer>(width, height, format, type, preciseFormat, GL_LINEAR, GL_CLAMP_TO_EDGE, false);
		_finalTexture = _finalFramebuffer->textureId();
	} else {
		_finalTexture = _frameBuffers[0]->textureId();
	}
	
	
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
	
	// Then iterate over all framebuffers, cascading down the texture.
	for(size_t i = 1; i < _frameBuffers.size(); ++i){
		_frameBuffers[i]->bind();
		_frameBuffers[i]->setViewport();
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(_passthroughProgram->id());
		ScreenQuad::draw(_frameBuffers[i-1]->textureId());
		_frameBuffers[i]->unbind();
	}
	
	// Blur vertically each framebuffer into frameBuffersBlur.
	for(size_t i = 0; i < _frameBuffers.size(); ++i){
		_frameBuffersBlur[i]->bind();
		_frameBuffersBlur[i]->setViewport();
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(_blurProgram->id());
		glUniform2f(_blurProgram->uniform("fetchOffset"), 0.0f, 1.2f/(float)_frameBuffersBlur[i]->height());
		ScreenQuad::draw(_frameBuffers[i]->textureId());
		_frameBuffersBlur[i]->unbind();
	}
	// Blur horizontally each framebufferBlur back into frameBuffers.
	for(size_t i = 0; i < _frameBuffersBlur.size(); ++i){
		_frameBuffers[i]->bind();
		_frameBuffers[i]->setViewport();
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(_blurProgram->id());
		glUniform2f(_blurProgram->uniform("fetchOffset"), 1.2f/(float)_frameBuffers[i]->width(), 0.0f);
		ScreenQuad::draw(_frameBuffersBlur[i]->textureId());
		_frameBuffers[i]->unbind();
	}
	
	if (_frameBuffers.size() == 1) {
		// No need to merge.
		return;
	}

	_finalFramebuffer->bind();
	_finalFramebuffer->setViewport();
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(_combineProgram->id());
	ScreenQuad::draw(_textures);
	_finalFramebuffer->unbind();

}


void GaussianBlur::clean() const {
	for(auto & frameBuffer : _frameBuffers){
		frameBuffer->clean();
	}
	for (auto & frameBuffer : _frameBuffersBlur) {
		frameBuffer->clean();
	}
	Blur::clean();
}


void GaussianBlur::resize(unsigned int width, unsigned int height){
	for(size_t i = 0; i < _frameBuffers.size(); ++i){
		const unsigned int hwidth = (unsigned int)(width/std::pow(2,i));
		const unsigned int hheight = (unsigned int)(height/std::pow(2,i));
		_frameBuffers[i]->resize(hwidth, hheight);
		_frameBuffersBlur[i]->resize(hwidth, hheight);
	}
	if (_frameBuffers.size() > 1) {
		_finalFramebuffer->resize(width, height);
	}
}

