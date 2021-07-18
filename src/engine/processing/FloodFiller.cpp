#include "processing/FloodFiller.hpp"
#include "graphics/ScreenQuad.hpp"
#include "graphics/GPU.hpp"

FloodFiller::FloodFiller(unsigned int width, unsigned int height) {

	_iterations = int(std::ceil(std::log2(std::max(width, height))));

	const Descriptor desc = {Layout::RG16UI, Filter::NEAREST_NEAREST, Wrap::REPEAT};
	_ping				  = std::unique_ptr<Framebuffer>(new Framebuffer(width, height, desc, false, "Flood fill ping"));
	_pong				  = std::unique_ptr<Framebuffer>(new Framebuffer(width, height, desc, false, "Flood fill pong"));
	_final				  = std::unique_ptr<Framebuffer>(new Framebuffer(width, height, {Layout::RGBA8, Filter::NEAREST_NEAREST, Wrap::CLAMP}, false, "Flood fill final"));

	_extract		= Resources::manager().getProgram2D("extract-seeds");
	_floodfill		= Resources::manager().getProgram2D("flood-fill");
	_compositeDist  = Resources::manager().getProgram2D("distance-seeds");
	_compositeColor = Resources::manager().getProgram2D("color-seeds");
}

void FloodFiller::process(const Texture * texture, Output mode) {

	extractAndPropagate(texture);

	GPU::setDepthState(false);
	GPU::setBlendState(false);
	GPU::setCullState(true, Faces::BACK);

	_final->bind(Framebuffer::Load::DONTCARE);
	_final->setViewport();

	if(mode == Output::COLOR) {
		_compositeColor->use();
		_compositeColor->textures({_ping->texture(), texture});
		ScreenQuad::draw();
	} else if(mode == Output::DISTANCE) {
		_compositeDist->use();
		_compositeDist->texture(_ping->texture(), 0);
		ScreenQuad::draw();
	}

}

void FloodFiller::extractAndPropagate(const Texture * texture) {
	// Render seed positions in a 2 channels framebuffer (each non-black pixel is a seed).
	GPU::setDepthState(false);
	GPU::setBlendState(false);
	GPU::setCullState(true, Faces::BACK);

	_ping->bind(Framebuffer::Load::DONTCARE);
	_ping->setViewport();
	_extract->use();
	_extract->texture(texture, 0);
	ScreenQuad::draw();

	// Propagate closest seeds with decreasing step size.
	_floodfill->use();
	for(int i = 0; i < _iterations; ++i) {
		const int step = int(std::pow(2, std::max(0, _iterations - i - 1)));
		_pong->bind(Framebuffer::Load::DONTCARE);
		_pong->setViewport();
		_floodfill->uniform("stepDist", step);
		_floodfill->texture(_ping->texture(), 0);
		ScreenQuad::draw();
		std::swap(_ping, _pong);
	}
}

void FloodFiller::resize(unsigned int width, unsigned int height) {
	_iterations = int(std::ceil(std::log2(std::max(width, height))));
	_ping->resize(width, height);
	_pong->resize(width, height);
	_final->resize(width, height);
}
