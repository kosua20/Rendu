#include "Animation.hpp"

Rotation::Rotation(const glm::vec3 & axis, float speed, Frame frame){
	_axis = glm::normalize(axis);
	_speed = speed;
	_frame = frame;
}

glm::mat4 Rotation::apply(const glm::mat4 & m, double fullTime, double frameTime){
	const glm::mat4 r = glm::rotate(glm::mat4(1.0f), _speed*float(frameTime), _axis);
	return (_frame == Frame::WORLD ? r*m : m*r);
}

glm::vec4 Rotation::apply(const glm::vec4 & v, double fullTime, double frameTime){
	const glm::mat4 r = glm::rotate(glm::mat4(1.0f), _speed*float(frameTime), _axis);
	return r*v;
}

BackAndForth::BackAndForth(const glm::vec3 & axis, float speed, float amplitude, Frame frame){
	_axis = glm::normalize(axis);
	_speed = speed;
	_amplitude = amplitude;
	_frame = frame;
}

glm::mat4 BackAndForth::apply(const glm::mat4 & m, double fullTime, double frameTime){
	const double currentAbscisse = std::sin(_speed * fullTime);
	const float delta = currentAbscisse - _previousAbscisse;
	_previousAbscisse = currentAbscisse;
	
	const glm::vec3 trans = delta * _amplitude * _axis;
	const glm::mat4 t = glm::translate(glm::mat4(1.0f), trans);
	return (_frame == Frame::WORLD ? t*m : m*t);
}

glm::vec4 BackAndForth::apply(const glm::vec4 & v, double fullTime, double frameTime){
	const double currentAbscisse = std::sin(_speed * fullTime);
	const float delta = currentAbscisse - _previousAbscisse;
	_previousAbscisse = currentAbscisse;
	
	const glm::vec3 trans = delta * _amplitude * _axis;
	
	return v + glm::vec4(trans, 0.0f);
}
