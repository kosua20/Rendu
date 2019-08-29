#include "processing/FloodFiller.hpp"
#include "graphics/ScreenQuad.hpp"

FloodFiller::FloodFiller(unsigned int width, unsigned int height) {
	
	_iterations = int(std::ceil(std::log2(std::max(width, height))));
	
	const Descriptor desc = {Layout::RG16UI, Filter::NEAREST_NEAREST, Wrap::REPEAT};
	_ping = std::unique_ptr<Framebuffer>(new Framebuffer(width, height, desc, false));
	_pong = std::unique_ptr<Framebuffer>(new Framebuffer(width, height, desc, false));
	_final = std::unique_ptr<Framebuffer>(new Framebuffer(width, height, {Layout::RGBA8, Filter::NEAREST_NEAREST, Wrap::CLAMP}, false));
	
	_extract = Resources::manager().getProgram2D("extract-seeds");
	_floodfill = Resources::manager().getProgram2D("flood-fill");
	_compositeDist = Resources::manager().getProgram2D("distance-seeds");
	_compositeColor = Resources::manager().getProgram2D("color-seeds");
}

void FloodFiller::process(const Texture * textureId, Output mode) {
	
	extractAndPropagate(textureId);
	
	_final->bind();
	_final->setViewport();
	
	if(mode == Output::COLOR){
		_compositeColor->use();
		ScreenQuad::draw({_ping->textureId(), textureId });
	} else if(mode == Output::DISTANCE){
		_compositeDist->use();
		ScreenQuad::draw(_ping->textureId());
	}
	
	_final->unbind();
}


void FloodFiller::extractAndPropagate(const Texture * textureId){
	// Render seed positions in a 2 channels framebuffer (each non-black pixel is a seed).
	glDisable(GL_DEPTH_TEST);
	_ping->bind();
	_ping->setViewport();
	_extract->use();
	ScreenQuad::draw(textureId);
	_ping->unbind();
	
	// Propagate closest seeds with decreasing step size.
	_floodfill->use();
	for(int i = 0; i < _iterations; ++i){
		const int step = int(std::pow(2, std::max(0, _iterations - i - 1)));
		_pong->bind();
		_pong->setViewport();
		_floodfill->uniform("stepDist", step);
		ScreenQuad::draw(_ping->textureId());
		_pong->unbind();
		std::swap(_ping, _pong);
	}
}

void FloodFiller::clean() const {
	_ping->clean();
	_pong->clean();
	_final->clean();
}


void FloodFiller::resize(unsigned int width, unsigned int height){
	_iterations = int(std::ceil(std::log2(std::max(width, height))));
	_ping->resize(width, height);
	_pong->resize(width, height);
	_final->resize(width, height);
}
