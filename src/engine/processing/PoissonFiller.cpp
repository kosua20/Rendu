#include "processing/PoissonFiller.hpp"
#include "graphics/GPU.hpp"

PoissonFiller::PoissonFiller(uint width, uint height, uint downscaling) :
	_pyramid(width / downscaling, height / downscaling, 0), _preproc("Poisson preproc"), _compo("Poisson compo"),
	_scale(int(downscaling)) {

	_prepare   = Resources::manager().getProgram2D("fill-boundary");
	_composite = Resources::manager().getProgram2D("fill-combine");

	_preproc.setupAsDrawable(Layout::RGBA32F, _pyramid.width(), _pyramid.height());
		_compo.setupAsDrawable(Layout::RGBA8, width, height);

	const float h1[5] = {0.1507f, 0.6836f, 1.0334f, 0.6836f, 0.1507f};
	const float h2	= 0.0270f;
	const float g[3]  = {0.0312f, 0.7753f, 0.0312f};
	_pyramid.setFilters(h1, h2, g);

}

void PoissonFiller::process(const Texture& texture) {
	// Compute the color boundary of the mask..
	GPU::setDepthState(false);
	GPU::setBlendState(false);
	GPU::setCullState(true, Faces::BACK);

	GPU::beginRender(glm::vec4(0.0f), &_preproc);
	GPU::setViewport(_preproc);
	_prepare->use();
	_prepare->texture(texture, 0);
	GPU::drawQuad();
	GPU::endRender();

	// Run the convolutional pyramid filter.
	_pyramid.process(_preproc);

	// Composite the filled-in texture with the initial image at full resolution.
	GPU::setDepthState(false);
	GPU::setBlendState(false);
	GPU::setCullState(true, Faces::BACK);

	GPU::beginRender(Load::Operation::DONTCARE, &_compo);
	GPU::setViewport(_compo);
	_composite->use();
	_composite->texture(_pyramid.texture(), 0);
	_composite->texture(texture, 1);
	GPU::drawQuad();
	GPU::endRender();
}

void PoissonFiller::resize(uint width, uint height) {
	_pyramid.resize(width / _scale, height / _scale);
	_preproc.resize(_pyramid.width(), _pyramid.height());
	_compo.resize(width, height);
}
