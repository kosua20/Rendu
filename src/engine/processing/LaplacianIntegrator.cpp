#include "LaplacianIntegrator.hpp"
#include "graphics/ScreenQuad.hpp"

LaplacianIntegrator::LaplacianIntegrator(unsigned int width, unsigned int height, unsigned int downscaling) : _pyramid(width / downscaling, height / downscaling, 1)  {
	
	// Pre and post process helpers.
	_prepare = Resources::manager().getProgram2D("laplacian");
	_composite = Resources::manager().getProgram2D("passthrough");
	
	const Descriptor desc = {GL_RGBA32F, GL_NEAREST, GL_CLAMP_TO_EDGE};
	_preproc = std::unique_ptr<Framebuffer>(new Framebuffer(_pyramid.width(), _pyramid.height(), desc , false));
	_compo   = std::unique_ptr<Framebuffer>(new Framebuffer(width, height, GL_RGBA8, false));
	_scale = downscaling;
	
	const float h1[5] = {0.15, 0.5, 0.7, 0.5, 0.15};
	const float h2 = 1.0;
	const float g[3] = {0.175, 0.547, 0.175};
	_pyramid.setFilters(h1, h2, g);
	
	checkGLError();
}

void LaplacianIntegrator::process(const GLuint textureId) {
	
	// First, compute the laplacian of each color channel (adding a 1px zero margin).
	glDisable(GL_DEPTH_TEST);
	glClearColor(0.0f,0.0f,0.0f,0.0f);
	_preproc->bind();
	_preproc->setViewport();
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(_prepare->id());
	glUniform1i(_prepare->uniform("scale"), _scale);
	ScreenQuad::draw(textureId);
	_preproc->unbind();
	
	// Run the convolutional pyramid filter.
	_pyramid.process(_preproc->textureId());
	
	// Upscale to the final resolution.
	_compo->bind();
	_compo->setViewport();
	glUseProgram(_composite->id());
	ScreenQuad::draw(_pyramid.textureId());
	_compo->unbind();
}

void LaplacianIntegrator::clean() const {
	_pyramid.clean();
	_compo->clean();
	_preproc->clean();
}

void LaplacianIntegrator::resize(unsigned int width, unsigned int height){
	_pyramid.resize(width/_scale, height/_scale);
	_preproc->resize(_pyramid.width(), _pyramid.height());
	_compo->resize(width, height);
}
