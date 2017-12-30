#include <GLFW/glfw3.h>
#include <stdio.h>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include "Input.hpp"
#include "Camera.hpp"


Camera::Camera()  {
	//_verticalResolution = 720;
	_speed = 1.2f;
	_angularSpeed = 4.0f;
	reset();
}

Camera::~Camera(){}

void Camera::reset(){
	_eye = glm::vec3(0.0,0.0,1.0);
	_center = glm::vec3(0.0,0.0,0.0);
	_up = glm::vec3(0.0,1.0,0.0);
	_right = glm::vec3(1.0,0.0,0.0);
	_view = glm::lookAt(_eye, _center, _up);
}

void Camera::update(double frameTime){
	if(Input::manager().triggered(Input::KeyR)){
		reset();
	}
	
	if (Input::manager().joystickAvailable()){
		updateUsingJoystick(frameTime);
	} else {
		updateUsingKeyboard(frameTime);
	}
	
	_view = glm::lookAt(_eye, _center, _up);
}

void Camera::updateUsingJoystick(double frameTime){
	const Joystick & joystick = Input::manager().joystick();
	// Handle buttons
	// Reset camera when pressing the Circle button.
	if(joystick.pressed(Joystick::ResetAll)){
		_eye = glm::vec3(0.0,0.0,1.0);
		_center = glm::vec3(0.0,0.0,0.0);
		_up = glm::vec3(0.0,1.0,0.0);
		_right = glm::vec3(1.0,0.0,0.0);
		return;
	}
	
	// Special actions to restore the camera orientation.
	// Restore the up vector.
	if(joystick.pressed(Joystick::ResetOrientation)){
		_up = glm::vec3(0.0f,1.0f,0.0f);
	}
	// Look at the center of the scene
	if( joystick.pressed(Joystick::ResetCenter)){
		_center[0] = _center[1] = _center[2] = 0.0f;
	}
	
	// The Up and Down boutons are configured to register each press only once
	// to avoid increasing/decreasing the speed for as long as the button is pressed.
	if(joystick.triggered(Joystick::SpeedUp)){
		_speed *= 2.0f;
	}
	
	if(joystick.triggered(Joystick::SpeedDown)){
		_speed *= 0.5f;
	}
	
	// Handle axis
	// Left stick to move
	// We need the direction of the camera, normalized.
	glm::vec3 look = normalize(_center - _eye);
	// Require a minimum deplacement between starting to register the move.
	const float axisForward = joystick.axis(Joystick::MoveForward);
	const float axisLateral = joystick.axis(Joystick::MoveLateral);
	const float axisUp = joystick.axis(Joystick::MoveUp);
	const float axisDown = joystick.axis(Joystick::MoveDown);
	const float axisVertical = joystick.axis(Joystick::LookVertical);
	const float axisHorizontal = joystick.axis(Joystick::LookHorizontal);
	
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

void Camera::updateUsingKeyboard(double frameTime){
	// We need the direction of the camera, normalized.
	glm::vec3 look = normalize(_center - _eye);
	// One step forward or backward.
	glm::vec3 deltaLook =  _speed * (float)frameTime * look;
	// One step laterally horizontal.
	glm::vec3 deltaLateral = _speed * (float)frameTime * _right;
	// One step laterally vertical.
	glm::vec3 deltaVertical = _speed * (float)frameTime * _up;
	
	
	if(Input::manager().pressed(Input::KeyW)){ // Forward
		_eye = _eye + deltaLook;
	}
	
	if(Input::manager().pressed(Input::KeyS)){ // Backward
		_eye = _eye - deltaLook;
	}
	
	if(Input::manager().pressed(Input::KeyA)){ // Left
		_eye = _eye - deltaLateral;
	}
	
	if(Input::manager().pressed(Input::KeyD)){ // Right
		_eye = _eye + deltaLateral;
	}
	
	if(Input::manager().pressed(Input::KeyQ)){ // Down
		_eye = _eye - deltaVertical;
	}
	
	if(Input::manager().pressed(Input::KeyE)){ // Up
		_eye = _eye + deltaVertical;
	}
	if(Input::manager().pressed(Input::MouseLeft)){
		/*glm::vec2 deltaPosition = Input::manager().moved(Input::MouseLeft) / screenSize();
		_center = _center + (deltaPosition.x * _right - deltaPosition.y * _up) * (float)frameTime * _angularSpeed;
		look = normalize(_center - _eye);*/
	}
	
	// Update center (eye-center stays constant).
	_center = _eye + look;
	
	// Recompute right as the cross product of look and up.
	_right = normalize(cross(look,_up));
	// Recompute up as the cross product of  right and look.
	_up = normalize(cross(_right,look));
	
}


void Camera::projection(float ratio, float fov){
	if(fov > 0.0){
		_fov = fov;
	}
	// Perspective projection.
	_projection = glm::perspective(_fov, ratio, 0.01f, 200.f);
}

/*
void Camera::internalResolution(int height){
	// No need to update the screen size.
	_verticalResolution = height;
	// Same aspect ratio as the display resolution
	_renderSize = (float(_verticalResolution)/_screenSize[1]) * _screenSize;
	// Perspective projection.
	_projection = glm::perspective(45.0f, _renderSize[0] / _renderSize[1], 0.01f, 200.f);
}
*/



