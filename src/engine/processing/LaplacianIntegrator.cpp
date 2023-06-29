#include "processing/LaplacianIntegrator.hpp"
#include "graphics/GPU.hpp"

LaplacianIntegrator::LaplacianIntegrator(uint width, uint height, uint downscaling) :
	_pyramid(width / downscaling, height / downscaling, 1), _preproc( "Laplacian preproc."), _compo("Laplacian compo"),
	_scale(int(downscaling)) {

	// Pre and post process helpers.
	_prepare   = Resources::manager().getProgram2D("laplacian");
	_composite = Resources::manager().getProgram2D("passthrough");

	_preproc.setupAsDrawable(Layout::RGBA32F, _pyramid.width(), _pyramid.height());
	_compo.setupAsDrawable(Layout::RGBA8, width, height);

	const float h1[5] = {0.15f, 0.5f, 0.7f, 0.5f, 0.15f};
	const float h2	= 1.0f;
	const float g[3]  = {0.175f, 0.547f, 0.175f};
	_pyramid.setFilters(h1, h2, g);

}

void LaplacianIntegrator::process(const Texture& texture) {

	// First, compute the laplacian of each color channel (adding a 1px zero margin).
	GPU::setDepthState(false);
	GPU::setBlendState(false);
	GPU::setCullState(true, Faces::BACK);

	GPU::bind(glm::vec4(0.0f), &_preproc);
	GPU::setViewport(_preproc);
	_prepare->use();
	_prepare->uniform("scale", _scale);
	_prepare->texture(texture, 0);
	GPU::drawQuad();

	// Run the convolutional pyramid filter.
	_pyramid.process(_preproc);

	// Upscale to the final resolution.
	GPU::setDepthState(false);
	GPU::setBlendState(false);
	GPU::setCullState(true, Faces::BACK);

	GPU::bind(Load::Operation::DONTCARE, &_compo);
	GPU::setViewport(_compo);
	_composite->use();
	_composite->texture(_pyramid.texture(), 0);
	GPU::drawQuad();
}

void LaplacianIntegrator::resize(uint width, uint height) {
	_pyramid.resize(width / _scale, height / _scale);
	_preproc.resize(_pyramid.width(), _pyramid.height());
	_compo.resize(width, height);
}
