#include "renderers/Renderer.hpp"
#include "input/Input.hpp"
#include "graphics/GLUtilities.hpp"
#include "graphics/Framebuffer.hpp"

Renderer::Renderer(RenderingConfig & config) : _config(config) {
	// Initial render resolution.
	_renderResolution = (float(_config.internalVerticalResolution)/_config.screenResolution[1]) * _config.screenResolution;
}

void Renderer::update(){
	// Handle window resize.
	if(Input::manager().resized()){
		resize(uint(Input::manager().size()[0]), uint(Input::manager().size()[1]));
	}
	// Perform screenshot capture in the current working directory.
	if(Input::manager().triggered(Input::KeyO) || (Input::manager().controllerAvailable() && Input::manager().controller()->triggered(Controller::ButtonView))){
		GLUtilities::saveFramebuffer(Framebuffer::backbuffer(), uint(_config.screenResolution[0]), uint(_config.screenResolution[1]), "./test-default", true, true);
	}
}


void Renderer::clean() {
	// Nothing to do here.
}


void Renderer::updateResolution(unsigned int width, unsigned int height){
	_config.screenResolution[0] = float(width > 0 ? width : 1);
	_config.screenResolution[1] = float(height > 0 ? height : 1);
	// Same aspect ratio as the display resolution
	_renderResolution = (float(_config.internalVerticalResolution)/_config.screenResolution[1]) * _config.screenResolution;
}

