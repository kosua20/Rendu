#include "processing/LaplacianIntegrator.hpp"
#include "graphics/ScreenQuad.hpp"
#include "graphics/GLUtilities.hpp"

LaplacianIntegrator::LaplacianIntegrator(unsigned int width, unsigned int height, unsigned int downscaling) :
	_pyramid(width / downscaling, height / downscaling, 1),
	_scale(int(downscaling)) {

	// Pre and post process helpers.
	_prepare   = Resources::manager().getProgram2D("laplacian");
	_composite = Resources::manager().getProgram2D("passthrough");

	const Descriptor descPrep = {Layout::RGBA32F, Filter::NEAREST_NEAREST, Wrap::CLAMP};
	const Descriptor descCompo = {Layout::RGBA8, Filter::LINEAR_NEAREST, Wrap::CLAMP};
	_preproc				   = std::unique_ptr<Framebuffer>(new Framebuffer(_pyramid.width(), _pyramid.height(), descPrep, false, "Laplacian preproc."));
	_compo				  = std::unique_ptr<Framebuffer>(new Framebuffer(width, height, descCompo, false, "Laplacian compo"));

	const float h1[5] = {0.15f, 0.5f, 0.7f, 0.5f, 0.15f};
	const float h2	= 1.0f;
	const float g[3]  = {0.175f, 0.547f, 0.175f};
	_pyramid.setFilters(h1, h2, g);

	checkGLError();
}

void LaplacianIntegrator::process(const Texture * texture) {

	// First, compute the laplacian of each color channel (adding a 1px zero margin).
	GLUtilities::setDepthState(false);

	_preproc->bind();
	_preproc->setViewport();
	GLUtilities::clearColor(glm::vec4(0.0f));
	_prepare->use();
	_prepare->uniform("scale", _scale);
	ScreenQuad::draw(texture);

	// Run the convolutional pyramid filter.
	_pyramid.process(_preproc->texture());

	// Upscale to the final resolution.
	_compo->bind();
	_compo->setViewport();
	_composite->use();
	ScreenQuad::draw(_pyramid.texture());
}

void LaplacianIntegrator::resize(unsigned int width, unsigned int height) {
	_pyramid.resize(width / _scale, height / _scale);
	_preproc->resize(_pyramid.width(), _pyramid.height());
	_compo->resize(width, height);
}
