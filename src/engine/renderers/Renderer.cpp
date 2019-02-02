#include "Renderer.hpp"
#include "../Object.hpp"
#include "../input/Input.hpp"


Renderer::Renderer(RenderingConfig & config) : _config(config) {
	
	// Initial render resolution.
	_renderResolution = (_config.internalVerticalResolution/_config.screenResolution[1]) * _config.screenResolution;
	
	defaultGLSetup();
	
}


void Renderer::update(){
	// Handle window resize.
	if(Input::manager().resized()){
		resize((unsigned int)Input::manager().size()[0], (unsigned int)Input::manager().size()[1]);
	}
	// Perform screenshot capture in the current working directory.
	if(Input::manager().triggered(Input::KeyO) || (Input::manager().controller() && Input::manager().controller()->triggered(Controller::ButtonView))){
		GLUtilities::saveDefaultFramebuffer((unsigned int)_config.screenResolution[0], (unsigned int)_config.screenResolution[1], "./test-default");
	}
}


void Renderer::clean() const {
	
}


void Renderer::updateResolution(unsigned int width, unsigned int height){
	_config.screenResolution[0] = float(width > 0 ? width : 1);
	_config.screenResolution[1] = float(height > 0 ? height : 1);
	// Same aspect ratio as the display resolution
	_renderResolution = (_config.internalVerticalResolution/_config.screenResolution[1]) * _config.screenResolution;
}

void Renderer::defaultGLSetup(){
	// Default GL setup.
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

Renderer::~Renderer(){
	
}

