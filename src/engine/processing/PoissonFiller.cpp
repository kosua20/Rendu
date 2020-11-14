#include "processing/PoissonFiller.hpp"
#include "graphics/ScreenQuad.hpp"
#include "graphics/GLUtilities.hpp"

PoissonFiller::PoissonFiller(unsigned int width, unsigned int height, unsigned int downscaling) :
	_pyramid(width / downscaling, height / downscaling, 0),
	_scale(int(downscaling)) {

	_prepare   = Resources::manager().getProgram2D("fill-boundary");
	_composite = Resources::manager().getProgram2D("fill-combine");

	const Descriptor descPrep = {Layout::RGBA32F, Filter::NEAREST_NEAREST, Wrap::CLAMP};
	const Descriptor descCompo = {Layout::RGBA8, Filter::LINEAR_NEAREST, Wrap::CLAMP};
	_preproc			  = std::unique_ptr<Framebuffer>(new Framebuffer(_pyramid.width(), _pyramid.height(), descPrep, false, "Poisson preproc"));
	_compo				  = std::unique_ptr<Framebuffer>(new Framebuffer(width, height, descCompo, false, "Poisson compo"));

	const float h1[5] = {0.1507f, 0.6836f, 1.0334f, 0.6836f, 0.1507f};
	const float h2	= 0.0270f;
	const float g[3]  = {0.0312f, 0.7753f, 0.0312f};
	_pyramid.setFilters(h1, h2, g);

	checkGLError();
}

void PoissonFiller::process(const Texture * texture) {
	// Compute the color boundary of the mask..
	GLUtilities::setDepthState(false);
	_preproc->bind();
	_preproc->setViewport();
	GLUtilities::clearColor(glm::vec4(0.0f));
	_prepare->use();
	ScreenQuad::draw(texture);

	// Run the convolutional pyramid filter.
	_pyramid.process(_preproc->texture());

	// Composite the filled-in texture with the initial image at full resolution.
	_compo->bind();
	_compo->setViewport();
	_composite->use();
	ScreenQuad::draw({_pyramid.texture(), texture});
}

void PoissonFiller::resize(unsigned int width, unsigned int height) {
	_pyramid.resize(width / _scale, height / _scale);
	_preproc->resize(_pyramid.width(), _pyramid.height());
	_compo->resize(width, height);
}
