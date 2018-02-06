#include "Camera.hpp"
#include "Input.hpp"
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#ifdef _WIN32
#define M_PI	3.14159265358979323846
#endif

Camera::Camera()  {
	//_verticalResolution = 720;
	_speed = 1.2f;
	_angularSpeed = 4.0f;
	_fov = 1.91f;
	_ratio = 4.0f/3.0f;
	_near = 0.01f;
	_far = 100.0f;
	_mode = TurnTable;
	reset();
}

Camera::~Camera(){}

void Camera::reset(){
	_eye = glm::vec3(0.0,0.0,1.0);
	_center = glm::vec3(0.0,0.0,0.0);
	_up = glm::vec3(0.0,1.0,0.0);
	_right = glm::vec3(1.0,0.0,0.0);
	_view = glm::lookAt(_eye, _center, _up);
	_verticalAngle = 0.0f;
	_horizontalAngle = (float)M_PI*0.5f;
	_radius = 1.0;
}

void Camera::update(){
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

void Camera::physics(double frameTime){
	
	if (Input::manager().joystickAvailable()){
		updateUsingJoystick(frameTime);
	} else if (_mode == FPS) {
		updateUsingKeyboard(frameTime);
	} else if (_mode == TurnTable){
		updateUsingTurnTable(frameTime);
	}
	
	_view = glm::lookAt(_eye, _center, _up);
}

void Camera::updateUsingJoystick(double frameTime){
	Joystick & joystick = Input::manager().joystick();
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
	
	// Update center (eye-center stays constant).
	_center = _eye + look;
	
	// Recompute right as the cross product of look and up.
	_right = normalize(cross(look,_up));
	// Recompute up as the cross product of  right and look.
	_up = normalize(cross(_right,look));
	
}

void Camera::updateUsingTurnTable(double frameTime){
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
	_horizontalAngle += delta[0] * (float)frameTime*_angularSpeed;
	_verticalAngle = (std::max)(-1.57f, (std::min)(1.57f, _verticalAngle + delta[1] * (float)frameTime*_angularSpeed));
	
	// Compute new look direction.
	const glm::vec3 newLook = - glm::vec3( cos(_verticalAngle) * cos(_horizontalAngle), sin(_verticalAngle),  cos(_verticalAngle) * sin(_horizontalAngle));
	
	// Update the camera position around the center.
	_eye =  _center - _radius * newLook;
	
	// Recompute right as the cross product of look and up.
	_right = normalize(cross(newLook,glm::vec3(0.0f,1.0f,0.0f)));
	// Recompute up as the cross product of  right and look.
	_up = normalize(cross(_right,newLook));
	
}

void Camera::projection(float ratio, float fov, float near, float far){
	_near = near;
	_far = far;
	_ratio = ratio;
	_fov = fov;
	updateProjection();
}

void Camera::frustum(float near, float far){
	_near = near;
	_far = far;
	updateProjection();
}

void Camera::ratio(float ratio){
	_ratio = ratio;
	updateProjection();
}

void Camera::fov(float fov){
	_fov = fov;
	updateProjection();
}

void Camera::updateProjection(){
	// Perspective projection.
	_projection = glm::perspective(_fov, _ratio, _near, _far);
}

