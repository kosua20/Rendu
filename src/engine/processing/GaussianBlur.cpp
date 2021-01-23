#include "processing/GaussianBlur.hpp"
#include "graphics/GLUtilities.hpp"
#include "graphics/ScreenQuad.hpp"
#include "resources/ResourcesManager.hpp"


GaussianBlur::GaussianBlur(uint radius, uint downscale, const std::string & name) : _name(name), _downscale(downscale) {
	_passthrough 		= Resources::manager().getProgram("passthrough");
	_blurProgramDown	= Resources::manager().getProgram2D("blur-dual-filter-down");
	_blurProgramUp		= Resources::manager().getProgram2D("blur-dual-filter-up");
	if(radius > 0){
		_frameBuffers = std::vector<std::unique_ptr<Framebuffer>>(radius);
	}
	checkGLError();
}

void GaussianBlur::process(const Texture * texture, Framebuffer & framebuffer) {
	if(_frameBuffers.empty()) {
		return;
	}

	GLUtilities::setDepthState(false);
	GLUtilities::setBlendState(false);
	GLUtilities::setCullState(true, Faces::BACK);

	const uint width = framebuffer.width() / _downscale;
	const uint height = framebuffer.height() / _downscale;
	if(!_frameBuffers[0] || _frameBuffers[0]->descriptor() != framebuffer.descriptor()){
		for(size_t i = 0; i < _frameBuffers.size(); ++i) {
			_frameBuffers[i] = std::unique_ptr<Framebuffer>(new Framebuffer(uint(width / std::pow(2, i)), uint(height / std::pow(2, i)), framebuffer.descriptor(), false, _name + " Gaussian blur level " + std::to_string(i)));
		}
	}
	if(_frameBuffers[0]->width() != width || _frameBuffers[0]->height() != height){
		resize(framebuffer.width(), framebuffer.height());
	}

	// First, copy the input texture to the first framebuffer.
	_frameBuffers[0]->bind();
	_frameBuffers[0]->setViewport();
	_passthrough->use();
	ScreenQuad::draw(texture);

	// Downscale filter.
	_blurProgramDown->use();
	for(size_t d = 1; d < _frameBuffers.size(); ++d) {
		_frameBuffers[d]->bind();
		_frameBuffers[d]->setViewport();
		GLUtilities::clearColor(glm::vec4(0.0f));
		ScreenQuad::draw(_frameBuffers[d - 1]->texture());
	}

	// Upscale filter.
	_blurProgramUp->use();
	for(int d = int(_frameBuffers.size()) - 2; d >= 0; --d) {
		_frameBuffers[d]->bind();
		_frameBuffers[d]->setViewport();
		GLUtilities::clearColor(glm::vec4(0.0f));
		ScreenQuad::draw(_frameBuffers[d + 1]->texture());
	}
	// Copy from the last framebuffer used to the destination.
	GLUtilities::blit(*_frameBuffers[0], framebuffer, Filter::LINEAR);
}

void GaussianBlur::resize(unsigned int width, unsigned int height) {
	const uint dwidth = width/_downscale;
	const uint dheight = height/_downscale;
	for(size_t i = 0; i < _frameBuffers.size(); ++i) {
		const unsigned int hwidth  = uint(dwidth / std::pow(2, i));
		const unsigned int hheight = uint(dheight / std::pow(2, i));
		_frameBuffers[i]->resize(hwidth, hheight);
	}
}
