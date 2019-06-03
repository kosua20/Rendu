#include "FloodFill.hpp"
#include "graphics/ScreenQuad.hpp"

FloodFilling::FloodFilling(unsigned int width, unsigned int height) {
	
	_iterations = std::ceil(std::log2(std::max(width, height)));
	
	const Descriptor desc = {GL_RG16UI, GL_NEAREST, GL_REPEAT};
	_ping = std::unique_ptr<Framebuffer>(new Framebuffer(width, height, desc, false));
	_pong = std::unique_ptr<Framebuffer>(new Framebuffer(width, height, desc, false));
	_final = std::unique_ptr<Framebuffer>(new Framebuffer(width, height, {GL_RGBA8, GL_NEAREST, GL_CLAMP_TO_EDGE}, false));
	
	_extract = Resources::manager().getProgram2D("extract-seeds");
	_floodfill = Resources::manager().getProgram2D("flood-fill");
	_composite = Resources::manager().getProgram2D("color-seeds"); // distance-seeds
}

void FloodFilling::process(const GLuint textureId) {
	// Render seed positions in a 2 channels framebuffer (each non-black pixel is a seed).
	glDisable(GL_DEPTH_TEST);
	_ping->bind();
	_ping->setViewport();
	glUseProgram(_extract->id());
	ScreenQuad::draw(textureId);
	glUseProgram(0);
	_ping->unbind();
	
	//
	glUseProgram(_floodfill->id());
	for(int i = 0; i < _iterations; ++i){
		const int step = std::pow(2, std::max(0, _iterations - i - 1));
		_pong->bind();
		_pong->setViewport();
		glUniform1i(_floodfill->uniform("stepDist"), step);
		ScreenQuad::draw(_ping->textureId());
		_pong->unbind();
		std::swap(_ping, _pong);
	}
	
	// The result is now in ping again.
	_final->bind();
	_final->setViewport();
	glUseProgram(_composite->id());
	ScreenQuad::draw({_ping->textureId(), textureId });
	_final->unbind();
	checkGLError();
}

void FloodFilling::clean() const {
	_ping->clean();
	_pong->clean();
	_final->clean();
}


void FloodFilling::resize(unsigned int width, unsigned int height){
	_iterations = std::ceil(std::log2(std::max(width, height)));
	_ping->resize(width, height);
	_pong->resize(width, height);
	_final->resize(width, height);
}
