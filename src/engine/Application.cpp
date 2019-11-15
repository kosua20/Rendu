#include "Application.hpp"
#include "input/Input.hpp"
#include "graphics/GLUtilities.hpp"
#include "graphics/Framebuffer.hpp"
#include "system/System.hpp"

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

CameraApp::CameraApp(RenderingConfig & config) : Application(config) {
	_timer = System::time();
	_userCamera.ratio(config.screenResolution[0] / config.screenResolution[1]);
}

void CameraApp::update(){
	Application::update();
	_userCamera.update();
	
	// We separate punctual events from the main physics/movement update loop.
	// Compute the time elapsed since last frame
	const double currentTime = System::time();
	double frameTime		 = currentTime - _timer;
	_timer					 = currentTime;
	
	// Physics simulation
	// First avoid super high frametime by clamping.
	if(frameTime > 0.2) {
		frameTime = 0.2;
	}
	// Accumulate new frame time.
	_remainingTime += frameTime;
	// Instead of bounding at dt, we lower our requirement (1 order of magnitude).
	while(_remainingTime > 0.2 * _dt) {
		const double deltaTime = std::min(_remainingTime, _dt);
		if(!_freezeCamera){
			_userCamera.physics(frameTime);
		}
		// Update physics.
		physics(_fullTime, deltaTime);
		// Update timers.
		_fullTime += deltaTime;
		_remainingTime -= deltaTime;
	}
}

void CameraApp::freezeCamera(bool shouldFreeze){
	_freezeCamera = shouldFreeze;
}
