#include "PoissonFiller.hpp"
#include "graphics/ScreenQuad.hpp"

PoissonFiller::PoissonFiller(unsigned int width, unsigned int height, unsigned int downscaling) : _pyramid(width / downscaling, height / downscaling, 0) {
	
	_scale = downscaling;
	
	_prepare = Resources::manager().getProgram2D("fill-boundary");
	_composite = Resources::manager().getProgram2D("fill-combine");
	
	const Descriptor desc = {GL_RGBA32F, GL_NEAREST, GL_CLAMP_TO_EDGE};
	_preproc = std::unique_ptr<Framebuffer>(new Framebuffer(_pyramid.width(), _pyramid.height(), desc , false));
	_compo   = std::unique_ptr<Framebuffer>(new Framebuffer(width, height, GL_RGBA8, false));
	
	const float h1[5] = {0.1507, 0.6836, 1.0334, 0.6836, 0.1507};
	const float h2 = 0.0270;
	const float g[3] = {0.0312, 0.7753,0.0312};
	_pyramid.setFilters(h1, h2, g);
	
	checkGLError();
}

void PoissonFiller::process(const GLuint textureId) {
	// Compute the color boundary of the mask..
	glDisable(GL_DEPTH_TEST);
	glClearColor(0.0f,0.0f,0.0f,0.0f);
	_preproc->bind();
	_preproc->setViewport();
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(_prepare->id());
	ScreenQuad::draw(textureId);
	_preproc->unbind();
	
	// Run the convolutional pyramid filter.
	_pyramid.process(_preproc->textureId());
	
	// Composite the filled-in texture with the initial image at full resolution.
	_compo->bind();
	_compo->setViewport();
	glUseProgram(_composite->id());
	ScreenQuad::draw({_pyramid.textureId(), textureId });
	_compo->unbind();
}

void PoissonFiller::clean() const {
	_pyramid.clean();
	_compo->clean();
	_preproc->clean();
}

void PoissonFiller::resize(unsigned int width, unsigned int height){
	_pyramid.resize(width/_scale, height/_scale);
	_preproc->resize(_pyramid.width(), _pyramid.height());
	_compo->resize(width, height);
}
