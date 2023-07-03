#include "processing/FloodFiller.hpp"
#include "graphics/GPU.hpp"

FloodFiller::FloodFiller(uint width, uint height)
	: _ping("Flood fill ping"), _pong( "Flood fill pong"), _final("Flood fill final") {

	_iterations = int(std::ceil(std::log2(std::max(width, height))));

	_ping.setupAsDrawable(Layout::RG16UI, width, height);
	_pong.setupAsDrawable(Layout::RG16UI, width, height);
	_final.setupAsDrawable(Layout::RGBA8, width, height);

	_extract		= Resources::manager().getProgram2D("extract-seeds");
	_floodfill		= Resources::manager().getProgram2D("flood-fill");
	_compositeDist  = Resources::manager().getProgram2D("distance-seeds");
	_compositeColor = Resources::manager().getProgram2D("color-seeds");
}

void FloodFiller::process(const Texture& texture, Output mode) {

	Texture* result = extractAndPropagate(texture);

	GPU::setDepthState(false);
	GPU::setBlendState(false);
	GPU::setCullState(true, Faces::BACK);

	GPU::bind(Load::Operation::DONTCARE, &_final);
	GPU::setViewport(_final);

	if(mode == Output::COLOR) {
		_compositeColor->use();
		_compositeColor->texture(texture, 0);
		_compositeColor->texture(*result, 1);
		GPU::drawQuad();
	} else if(mode == Output::DISTANCE) {
		_compositeDist->use();
		_compositeDist->texture(*result, 0);
		GPU::drawQuad();
	}

}

Texture* FloodFiller::extractAndPropagate(const Texture& texture) {
	// Render seed positions in a 2 channels texture (each non-black pixel is a seed).
	GPU::setDepthState(false);
	GPU::setBlendState(false);
	GPU::setCullState(true, Faces::BACK);

	GPU::bind(Load::Operation::DONTCARE, &_ping);
	GPU::setViewport(_ping);
	_extract->use();
	_extract->texture(texture, 0);
	GPU::drawQuad();

	Texture* result = &_ping;
	// Propagate closest seeds with decreasing step size.
	_floodfill->use();
	for(int i = 0; i < _iterations; ++i) {
		const int step = int(std::pow(2, std::max(0, _iterations - i - 1)));

		Texture* src = (i%2 == 0) ? &_ping : &_pong;
		Texture* dst = (i%2 == 0) ? &_pong : &_ping;
		GPU::bind(Load::Operation::DONTCARE, dst);
		GPU::setViewport(*dst);
		_floodfill->uniform("stepDist", step);
		_floodfill->texture(*src, 0);
		GPU::drawQuad();
		result = dst;
	}
	return result;
}

void FloodFiller::resize(uint width, uint height) {
	_iterations = int(std::ceil(std::log2(std::max(width, height))));
	_ping.resize(width, height);
	_pong.resize(width, height);
	_final.resize(width, height);
}
