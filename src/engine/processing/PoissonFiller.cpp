#include "processing/PoissonFiller.hpp"
#include "graphics/ScreenQuad.hpp"
#include "graphics/GPU.hpp"

PoissonFiller::PoissonFiller(unsigned int width, unsigned int height, unsigned int downscaling) :
	_pyramid(width / downscaling, height / downscaling, 0),
	_scale(int(downscaling)) {

	_prepare   = Resources::manager().getProgram2D("fill-boundary");
	_composite = Resources::manager().getProgram2D("fill-combine");

	_preproc = std::unique_ptr<Framebuffer>(new Framebuffer(_pyramid.width(), _pyramid.height(), Layout::RGBA32F, "Poisson preproc"));
	_compo   = std::unique_ptr<Framebuffer>(new Framebuffer(width, height, Layout::RGBA8, "Poisson compo"));

	const float h1[5] = {0.1507f, 0.6836f, 1.0334f, 0.6836f, 0.1507f};
	const float h2	= 0.0270f;
	const float g[3]  = {0.0312f, 0.7753f, 0.0312f};
	_pyramid.setFilters(h1, h2, g);

}

void PoissonFiller::process(const Texture * texture) {
	// Compute the color boundary of the mask..
	GPU::setDepthState(false);
	GPU::setBlendState(false);
	GPU::setCullState(true, Faces::BACK);

	_preproc->bind(glm::vec4(0.0f), Load::Operation::DONTCARE, Load::Operation::DONTCARE);
	_preproc->setViewport();
	_prepare->use();
	_prepare->texture(texture, 0);
	ScreenQuad::draw();

	// Run the convolutional pyramid filter.
	_pyramid.process(_preproc->texture());

	// Composite the filled-in texture with the initial image at full resolution.
	GPU::setDepthState(false);
	GPU::setBlendState(false);
	GPU::setCullState(true, Faces::BACK);

	_compo->bind(Load::Operation::DONTCARE, Load::Operation::DONTCARE, Load::Operation::DONTCARE);
	_compo->setViewport();
	_composite->use();
	_composite->texture(_pyramid.texture(), 0);
	_composite->texture(texture, 1);
	ScreenQuad::draw();
}

void PoissonFiller::resize(unsigned int width, unsigned int height) {
	_pyramid.resize(width / _scale, height / _scale);
	_preproc->resize(_pyramid.width(), _pyramid.height());
	_compo->resize(width, height);
}
