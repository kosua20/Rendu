#include "ControllableCamera.hpp"
#include "Input.hpp"
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#ifdef _WIN32
#define M_PI	3.14159265358979323846
#endif

ControllableCamera::ControllableCamera() : Camera() {
	_speed = 1.2f;
	_angularSpeed = 4.0f;
	_mode = TurnTable;
	reset();
}

ControllableCamera::~ControllableCamera(){}

void ControllableCamera::reset(){
	_eye = glm::vec3(0.0,0.0,1.0);
	_center = glm::vec3(0.0,0.0,0.0);
	_up = glm::vec3(0.0,1.0,0.0);
	_right = glm::vec3(1.0,0.0,0.0);
	_view = glm::lookAt(_eye, _center, _up);
	_radius = 1.0;
	_angles = glm::vec2((float)M_PI*0.5f, 0.0f);
}

void ControllableCamera::update(){
	if(Input::manager().triggered(Input::KeyR)){
		reset();
	}
	if(Input::manager().triggered(Input::KeyF)){
		_mode = FPS;
	}
	if(Input::manager().triggered(Input::KeyG)){
		_mode = TurnTable;
		_radius = glm::length(_eye - _center);
	}
}

void ControllableCamera::physics(double frameTime){
	
	if (Input::manager().controllerAvailable()){
		updateUsingJoystick(frameTime);
	} else if (_mode == FPS) {
		updateUsingKeyboard(frameTime);
	} else if (_mode == TurnTable){
		updateUsingTurnTable(frameTime);
	}
	
	updateView();
}

void ControllableCamera::updateUsingJoystick(double frameTime){
	Controller & joystick = Input::manager().controller();
	// Handle buttons
	// Reset camera when pressing the Circle button.
	if(joystick.pressed(Controller::ButtonB)){
		_eye = glm::vec3(0.0,0.0,1.0);
		_center = glm::vec3(0.0,0.0,0.0);
		_up = glm::vec3(0.0,1.0,0.0);
		_right = glm::vec3(1.0,0.0,0.0);
		return;
	}
	
	// Special actions to restore the camera orientation.
	// Restore the up vector.
	if(joystick.pressed(Controller::BumperL1)){
		_up = glm::vec3(0.0f,1.0f,0.0f);
	}
	// Look at the center of the scene
	if( joystick.pressed(Controller::BumperR1)){
		_center[0] = _center[1] = _center[2] = 0.0f;
	}
	
	// The Up and Down boutons are configured to register each press only once
	// to avoid increasing/decreasing the speed for as long as the button is pressed.
	if(joystick.triggered(Controller::ButtonUp)){
		_speed *= 2.0f;
	}
	
	if(joystick.triggered(Controller::ButtonDown)){
		_speed *= 0.5f;
	}
	
	// Handle axis
	// Left stick to move
	// We need the direction of the camera, normalized.
	glm::vec3 look = normalize(_center - _eye);
	// Require a minimum deplacement between starting to register the move.
	const float axisForward = joystick.axis(Controller::PadLeftY);
	const float axisLateral = joystick.axis(Controller::PadLeftX);
	const float axisUp = joystick.axis(Controller::TriggerL2);
	const float axisDown = joystick.axis(Controller::TriggerR2);
	const float axisVertical = joystick.axis(Controller::PadRightY);
	const float axisHorizontal = joystick.axis(Controller::PadRightX);
	
	if(axisForward * axisForward + axisLateral * axisLateral > 0.1){
		// Update the camera position.
		_eye = _eye - axisForward * (float)frameTime * _speed * look;
		_eye = _eye + axisLateral * (float)frameTime * _speed * _right;
	}
	
	// L2 and R2 triggers are used to move up and down. They can be read like axis.
	if(axisUp > -0.9){
		_eye = _eye  - (axisUp + 1.0f)* 0.5f * (float)frameTime * _speed * _up;
	}
	if(axisDown > -0.9){
		_eye = _eye  + (axisDown + 1.0f)* 0.5f * (float)frameTime * _speed * _up;
	}
	
	// Update center (eye-center stays constant).
	_center = _eye + look;
	
	// Right stick to look around.
	if(axisVertical * axisVertical + axisHorizontal * axisHorizontal > 0.1){
		_center = _center - axisVertical * (float)frameTime * _angularSpeed * _up;
		_center = _center + axisHorizontal * (float)frameTime * _angularSpeed * _right;
	}
	// Renormalize the look vector.
	look = normalize(_center - _eye);
	// Recompute right as the cross product of look and up.
	_right = normalize(cross(look,_up));
	// Recompute up as the cross product of  right and look.
	_up = normalize(cross(_right,look));
}

void ControllableCamera::updateUsingKeyboard(double frameTime){
	// We need the direction of the camera, normalized.
	glm::vec3 look = normalize(_center - _eye);
	// One step forward or backward.
	const glm::vec3 deltaLook =  _speed * (float)frameTime * look;
	// One step laterally horizontal.
	const glm::vec3 deltaLateral = _speed * (float)frameTime * _right;
	// One step laterally vertical.
	const glm::vec3 deltaVertical = _speed * (float)frameTime * _up;
	
	
	if(Input::manager().pressed(Input::KeyW)){ // Forward
		_eye += deltaLook;
	}
	
	if(Input::manager().pressed(Input::KeyS)){ // Backward
		_eye -= deltaLook;
	}
	
	if(Input::manager().pressed(Input::KeyA)){ // Left
		_eye -= deltaLateral;
	}
	
	if(Input::manager().pressed(Input::KeyD)){ // Right
		_eye += deltaLateral;
	}
	
	if(Input::manager().pressed(Input::KeyQ)){ // Down
		_eye -= deltaVertical;
	}
	
	if(Input::manager().pressed(Input::KeyE)){ // Up
		_eye += deltaVertical;
	}
	
	
	const glm::vec2 delta = Input::manager().moved(Input::MouseLeft);
	_angles += delta* (float)frameTime*_angularSpeed;
	_angles[1] = (std::max)(-1.57f, (std::min)(1.57f, _angles[1]));
	// Right stick to look around.
	const glm::mat4 rotY = glm::rotate(glm::mat4(1.0f), (float)M_PI*0.5f-_angles[0], glm::vec3(0.0,1.0,0.0));
	const glm::mat4 rotX = glm::rotate(glm::mat4(1.0f), -_angles[1], glm::vec3(1.0,0.0,0.0));
	const glm::mat3 rot = glm::mat3(rotY * rotX);
	
	look = rot * glm::vec3(0.0,0.0,-1.0);
	_center = _eye + look;
	_up = rot * glm::vec3(0.0,1.0,0.0);
	_right = rot * glm::vec3(1.0,0.0,0.0);

	
}

void ControllableCamera::updateUsingTurnTable(double frameTime){
	// We need the direction of the camera, normalized.
	const glm::vec3 look = normalize(_center - _eye);
	// One step forward or backward.
	const glm::vec3 deltaLook =  _speed * (float)frameTime * look;
	// One step laterally horizontal.
	const glm::vec3 deltaLateral = _speed * (float)frameTime * _right;
	// One step laterally vertical.
	const glm::vec3 deltaVertical = _speed * (float)frameTime * _up;
	
	
	if(Input::manager().pressed(Input::KeyW)){ // Forward
		_center += deltaLook;
	}
	
	if(Input::manager().pressed(Input::KeyS)){ // Backward
		_center -= deltaLook;
	}
	
	if(Input::manager().pressed(Input::KeyA)){ // Left
		_center -= deltaLateral;
	}
	
	if(Input::manager().pressed(Input::KeyD)){ // Right
		_center += deltaLateral;
	}
	
	if(Input::manager().pressed(Input::KeyQ)){ // Down
		_center -= deltaVertical;
	}
	
	if(Input::manager().pressed(Input::KeyE)){ // Up
		_center += deltaVertical;
	}
	
	// Radius of the turntable.
	float scroll = Input::manager().scroll()[1];
	_radius = (std::max)(0.0001f, _radius - scroll * (float)frameTime*_speed);
	
	// Angles update for the turntable.
	const glm::vec2 delta = Input::manager().moved(Input::MouseLeft);
	_angles += delta* (float)frameTime*_angularSpeed;
	_angles[1] = (std::max)(-1.57f, (std::min)(1.57f, _angles[1]));
	
	// Compute new look direction.
	const glm::vec3 newLook = - glm::vec3( cos(_angles[1]) * cos(_angles[0]), sin(_angles[1]),  cos(_angles[1]) * sin(_angles[0]));
	
	// Update the camera position around the center.
	_eye =  _center - _radius * newLook;
	
	// Recompute right as the cross product of look and up.
	_right = normalize(cross(newLook,glm::vec3(0.0f,1.0f,0.0f)));
	// Recompute up as the cross product of  right and look.
	_up = normalize(cross(_right,newLook));
	
}
