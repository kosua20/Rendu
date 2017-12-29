#include <GLFW/glfw3.h>
#include <stdio.h>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include "Input.h"
#include "Camera.h"


Camera::Camera()  {
	_verticalResolution = 720;
	_speed = 1.2f;
	_angularSpeed = 0.7f;
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
	
	if (Input::manager().useJoystick()){
		updateUsingJoystick(frameTime);
	} else {
		updateUsingKeyboard(frameTime);
	}
	
	_view = glm::lookAt(_eye, _center, _up);
//	if(_joystick.id() >= 0){
//		// If a joystick is present, update it.
//		//_joystick.update(frameTime);
//	} else {
//		// Else update the keyboard.
//		_keyboard.update(frameTime);
//	}
//	// Update the view matrix.
//
}

void Camera::updateUsingJoystick(double frameTime){
	
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
		/*const glm::vec2 deltaPosition = Input::manager().moved(Input::MouseLeft);
		_center = _center + (deltaPosition.x * _right + deltaPosition.y * _up) * (float)frameTime * _angularSpeed;
		look = normalize(_center - _eye);*/
	}
	
	// Update center (eye-center stays constant).
	_center = _eye + look;
	
	// Recompute right as the cross product of look and up.
	_right = normalize(cross(look,_up));
	// Recompute up as the cross product of  right and look.
	_up = normalize(cross(_right,look));
}


void Camera::key(int key, bool flag){
	// Ignore if joystick present, for now.
//	if(_joystick.id() >= 0) {
//		return;
//	}
//
//	if (key == GLFW_KEY_W || key == GLFW_KEY_A
//		|| key == GLFW_KEY_S || key == GLFW_KEY_D
//		|| key == GLFW_KEY_Q || key == GLFW_KEY_E) {
//		_keyboard.registerMove(key, flag);
//	} else if(flag && key == GLFW_KEY_R) {
//		reset();
//	}
}

void Camera::joystick(int joystick, int event){
//	if (event == GLFW_CONNECTED) {
//		std::cout << "Connected joystick " << joystick << std::endl;
//		// If there is no currently connected joystick, register the new one.
//		if(_joystick.id() == -1){
//			_joystick.activate(joystick);
//		}
//	} else if (event == GLFW_DISCONNECTED) {
//		std::cout << "Disconnected joystick " << joystick << std::endl;
//		// If the disconnected joystick is the one currently used, register this.
//		if(joystick == _joystick.id()){
//			_joystick.deactivate();
//		}
//	}
}

void Camera::mouse(MouseMode mode, float x, float y){
	// Ignore if joystick present, for now.
//	if(_joystick.id() >= 0) {
//		return;
//	}
//
//	if (mode == MouseMode::End) {
//		_keyboard.endLeftMouse();
//	} else {
//		// We normalize the x and y values to the [-1, 1] range.
//		float xPosition =  fmax(fmin(1.0f, 2.0f * x / _screenSize[0] - 1.0f),-1.0f);
//		float yPosition =  fmax(fmin(1.0f, 2.0f * y / _screenSize[1] - 1.0f),-1.0f);
//
//		if(mode == MouseMode::Start) {
//			_keyboard.startLeftMouse(xPosition,yPosition);
//		} else {
//			_keyboard.leftMouseTo(xPosition,yPosition);
//		}
//	}
}


void Camera::screen(int width, int height){
	_screenSize[0] = float(width > 0 ? width : 1);
	_screenSize[1] = float(height > 0 ? height : 1);
	// Same aspect ratio as the display resolution
	_renderSize = (float(_verticalResolution)/_screenSize[1]) * _screenSize;
	// Perspective projection.
	_projection = glm::perspective(45.0f, _renderSize[0] / _renderSize[1], 0.01f, 200.f);
}

void Camera::internalResolution(int height){
	// No need to update the screen size.
	_verticalResolution = height;
	// Same aspect ratio as the display resolution
	_renderSize = (float(_verticalResolution)/_screenSize[1]) * _screenSize;
	// Perspective projection.
	_projection = glm::perspective(45.0f, _renderSize[0] / _renderSize[1], 0.01f, 200.f);
}




