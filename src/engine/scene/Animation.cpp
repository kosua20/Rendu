#include "Animation.hpp"

std::vector<std::shared_ptr<Animation>> Animation::decode(const std::vector<KeyValues> & params){
	std::vector<std::shared_ptr<Animation>> animations;
	// Each element is an animation.
	for(const auto & param : params){
		if(param.key == "rotation"){
			auto anim = std::shared_ptr<Rotation>(new Rotation());
			anim->decode(param);
			animations.push_back(anim);
			
		} else if(param.key == "backandforth"){
			auto anim = std::shared_ptr<BackAndForth>(new BackAndForth());
			anim->decode(param);
			animations.push_back(anim);
			
		}
	}
	return animations;
}

void Animation::decodeBase(const KeyValues & params){
	if(params.values.size() >= 2){
		const float speed  = std::stof(params.values[0]);
		const Animation::Frame frame = (params.values[1] == "model" ? Animation::Frame::MODEL : Animation::Frame::WORLD);
		_speed = speed;
		_frame = frame;
	}
}

Rotation::Rotation(){
}

Rotation::Rotation(const glm::vec3 & axis, float speed, Frame frame){
	_axis = glm::normalize(axis);
	_speed = speed;
	_frame = frame;
}

glm::mat4 Rotation::apply(const glm::mat4 & m, double, double frameTime){
	const glm::mat4 r = glm::rotate(glm::mat4(1.0f), _speed*float(frameTime), _axis);
	return (_frame == Frame::WORLD ? r*m : m*r);
}

glm::vec4 Rotation::apply(const glm::vec4 & v, double, double frameTime){
	const glm::mat4 r = glm::rotate(glm::mat4(1.0f), _speed*float(frameTime), _axis);
	return r*v;
}

void Rotation::decode(const KeyValues & params){
	Animation::decodeBase(params);
	_axis = Codable::decodeVec3(params, 2);
	_axis = glm::normalize(_axis);
}

BackAndForth::BackAndForth(){
	
}

BackAndForth::BackAndForth(const glm::vec3 & axis, float speed, float amplitude, Frame frame){
	_axis = glm::normalize(axis);
	_speed = speed;
	_amplitude = amplitude;
	_frame = frame;
}

glm::mat4 BackAndForth::apply(const glm::mat4 & m, double fullTime, double){
	const double currentAbscisse = std::sin(_speed * fullTime);
	const float delta = float(currentAbscisse - _previousAbscisse);
	_previousAbscisse = currentAbscisse;
	
	const glm::vec3 trans = delta * _amplitude * _axis;
	const glm::mat4 t = glm::translate(glm::mat4(1.0f), trans);
	return (_frame == Frame::WORLD ? t*m : m*t);
}

glm::vec4 BackAndForth::apply(const glm::vec4 & v, double fullTime, double){
	const double currentAbscisse = std::sin(_speed * fullTime);
	const float delta = float(currentAbscisse - _previousAbscisse);
	_previousAbscisse = currentAbscisse;
	
	const glm::vec3 trans = delta * _amplitude * _axis;
	return v + glm::vec4(trans, 0.0f);
}

void BackAndForth::decode(const KeyValues & params){
	Animation::decodeBase(params);
	_axis = Codable::decodeVec3(params, 2);
	_axis = glm::normalize(_axis);
	if(params.values.size() >= 6){
		_amplitude = std::stof(params.values[5]);
	}
}
