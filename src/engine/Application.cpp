#include "Application.hpp"
#include "input/Input.hpp"
#include "graphics/GLUtilities.hpp"
#include "graphics/Framebuffer.hpp"
#include "resources/ResourcesManager.hpp"
#include "system/System.hpp"

Application::Application(RenderingConfig & config) :
	_config(config) {
	_startTime = System::time();
	_timer = _startTime;
	for(uint fid = 0; fid < _frameTimes.size(); ++fid){
		_frameTimes[fid] = 0.0f;
	}

	if(_config.trackDebug) {
		_debug.reset(new DebugViewer(false));
		DebugViewer::setDefault(_debug.get());
	} else {
		// Create a silent viewer.
		_debug.reset(new DebugViewer(true));
	}
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
		const std::string filename = System::timestamp();
		GLUtilities::saveFramebuffer(*Framebuffer::backbuffer(), uint(_config.screenResolution[0]), uint(_config.screenResolution[1]), "./" + filename, true, true);
	}
	// Reload resources.
	if(Input::manager().triggered(Input::Key::P)) {
		Resources::manager().reload();
	}

	// Display debug informations.
	if(Input::manager().pressed(Input::Key::LeftControl) && Input::manager().triggered(Input::Key::Tab)) {
		_showDebug = !_showDebug;
	}
	if(_showDebug && _debug) {
		_debug->interface();
	}

	// Compute the time elapsed since last frame
	const double currentTime = System::time();
	_frameTime		 = currentTime - _timer;
	_timer			 = currentTime;

	// Keep accumulating smoothed time.
	_smoothTime -= _frameTimes[_currFrame];
	_frameTimes[_currFrame] = _frameTime;
	_smoothTime += _frameTimes[_currFrame];
	_currFrame = (_currFrame + 1) % _framesCount;
}

double Application::timeElapsed(){
	return _timer - _startTime;
}

double Application::frameTime(){
	return _frameTime;
}

double Application::frameRate(){
	return double(_framesCount) / _smoothTime;
}

CameraApp::CameraApp(RenderingConfig & config) : Application(config) {
	_userCamera.ratio(config.screenResolution[0] / config.screenResolution[1]);
}

void CameraApp::update(){
	Application::update();
	_userCamera.update();
	
	// We separate punctual events from the main physics/movement update loop.
	// Physics simulation
	// First avoid super high frametime by clamping.
	const double frameTimeUpdate = std::min(frameTime(), 0.2);
	// Accumulate new frame time.
	_remainingTime += frameTimeUpdate;
	// Instead of bounding at dt, we lower our requirement (1 order of magnitude).
	while(_remainingTime > 0.2 * _dt) {
		const double deltaTime = std::min(_remainingTime, _dt);
		if(!_freezeCamera){
			_userCamera.physics(deltaTime);
		}
		// Update physics.
		physics(_fullTime, deltaTime);
		// Update timers.
		_fullTime += deltaTime;
		_remainingTime -= deltaTime;
	}
}

void CameraApp::physics(double, double){
	
}

void CameraApp::freezeCamera(bool shouldFreeze){
	_freezeCamera = shouldFreeze;
}
