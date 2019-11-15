#include "Application.hpp"
#include "input/Input.hpp"
#include "graphics/GLUtilities.hpp"
#include "graphics/Framebuffer.hpp"

Application::Application(RenderingConfig & config) :
	_config(config) {
}

void Application::update() {
	// Handle window resize.
	if(Input::manager().resized()) {
		const unsigned int width = uint(Input::manager().size()[0]);
		const unsigned int height = uint(Input::manager().size()[1]);
		_config.screenResolution[0] = float(width > 0 ? width : 1);
		_config.screenResolution[1] = float(height > 0 ? height : 1);
		resize();
	}
	// Perform screenshot capture in the current working directory.
	if(Input::manager().triggered(Input::Key::O) || (Input::manager().controllerAvailable() && Input::manager().controller()->triggered(Controller::ButtonView))) {
		//\todo Generate timestamp.
		GLUtilities::saveFramebuffer(Framebuffer::backbuffer(), uint(_config.screenResolution[0]), uint(_config.screenResolution[1]), "./test-default", true, true);
	}
}
