#include "TestRenderer.hpp"
#include "../../input/Input.hpp"
#include <stdio.h>
#include <vector>


TestRenderer::~TestRenderer(){}

TestRenderer::TestRenderer(Config & config) : Renderer(config) {
	
	
	_framebuffer = std::make_shared<Framebuffer>(_renderResolution[0], _renderResolution[1], GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA, GL_LINEAR,GL_CLAMP_TO_EDGE, false);
	checkGLError();
	
	// GL options
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glBlendEquation (GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);
	checkGLError();

	_screenQuad.init("passthrough");
	checkGLError();
	
}


void TestRenderer::draw() {
	
	_framebuffer->bind();
	glClearColor(1.0f,0.0f,0.0f,1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glViewport(0,0,_framebuffer->width(), _framebuffer->height());
	_screenQuad.draw(Resources::manager().getTexture("desk_albedo").id);
	_framebuffer->unbind();
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_FRAMEBUFFER_SRGB);
	glViewport(0, 0, GLsizei(_config.screenResolution[0]), GLsizei(_config.screenResolution[1]));
	_screenQuad.draw(_framebuffer->textureId());
	glDisable(GL_FRAMEBUFFER_SRGB);
	
}

void TestRenderer::update(){
	Renderer::update();
	
	
	if(Input::manager().triggered(Input::KeyO)){
		GLUtilities::saveDefaultFramebuffer((unsigned int)_config.screenResolution[0], (unsigned int)_config.screenResolution[1], "./test-default");
	}
}

void TestRenderer::physics(double fullTime, double frameTime){
	
}


void TestRenderer::clean() const {
	Renderer::clean();
	// Clean objects.
	_framebuffer->clean();
	_screenQuad.clean();
}


void TestRenderer::resize(int width, int height){
	Renderer::updateResolution(width, height);
	_framebuffer->resize(_renderResolution);
}



