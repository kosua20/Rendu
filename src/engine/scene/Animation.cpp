#include "scene/Animation.hpp"

Animation::Animation(Frame frame, float speed) :
	_frame(frame), _speed(speed) {
}

std::vector<std::shared_ptr<Animation>> Animation::decode(const std::vector<KeyValues> & params) {
	std::vector<std::shared_ptr<Animation>> animations;
	// Each element is an animation.
	for(const auto & param : params) {
		if(param.key == "rotation") {
			auto anim = std::shared_ptr<Rotation>(new Rotation());
			if(anim->decode(param)){
				animations.push_back(anim);
			} else {
				Log::Warning() << "Failed to load " << param.key << " animation." << std::endl;
			}

		} else if(param.key == "backandforth") {
			auto anim = std::shared_ptr<BackAndForth>(new BackAndForth());
			if(anim->decode(param)){
				animations.push_back(anim);
			} else {
				Log::Warning() << "Failed to load " << param.key << " animation." << std::endl;
			}
		} else {
			Codable::unknown(param);
		}
	}
	return animations;
}

std::vector<KeyValues> Animation::encode(const std::vector<std::shared_ptr<Animation>> & anims)  {
	std::vector<KeyValues> res;
	for(size_t i = 0; i < anims.size(); ++i){
		res.push_back(anims[i]->encode());
	}
	return res;
}

bool Animation::decodeBase(const KeyValues & params) {
	if(params.values.size() >= 2) {
		const float speed			 = std::stof(params.values[0]);
		const Animation::Frame frame = (params.values[1] == "model" ? Animation::Frame::MODEL : Animation::Frame::WORLD);
		_speed						 = speed;
		_frame						 = frame;
	}
	return true;
}

KeyValues Animation::encode() const {
	KeyValues anim("anim");
	anim.values.emplace_back(std::to_string(_speed));
	anim.values.emplace_back(_frame == Frame::MODEL ? "model" : "world");
	return anim;
}

Rotation::Rotation(const glm::vec3 & axis, float speed, Frame frame) :
	Animation(frame, speed),
	_axis(glm::normalize(axis)) {
}

glm::mat4 Rotation::apply(const glm::mat4 & m, double, double frameTime) {
	const glm::mat4 r = glm::rotate(glm::mat4(1.0f), _speed * float(frameTime), _axis);
	return (_frame == Frame::WORLD ? r * m : m * r);
}

glm::vec4 Rotation::apply(const glm::vec4 & v, double, double frameTime) {
	const glm::mat4 r = glm::rotate(glm::mat4(1.0f), _speed * float(frameTime), _axis);
	return r * v;
}

bool Rotation::decode(const KeyValues & params) {
	bool success = Animation::decodeBase(params);
	// Load axis and validate.
	const glm::vec3 newAxis = Codable::decodeVec3(params, 2);
	if(newAxis == glm::vec3(0.0f)){
		return false;
	}
	_axis = glm::normalize(newAxis);
	return success;
}

KeyValues Rotation::encode() const {
	KeyValues base = Animation::encode();
	base.key = "rotation";
	auto axis = Codable::encode(_axis);
	base.values.insert(base.values.end(), axis.begin(), axis.end());
	return base;
}

BackAndForth::BackAndForth(const glm::vec3 & axis, float speed, float amplitude, Frame frame) :
	Animation(frame, speed),
	_axis(glm::normalize(axis)), _amplitude(amplitude) {
}

glm::mat4 BackAndForth::apply(const glm::mat4 & m, double fullTime, double) {
	const double currentAbscisse = std::sin(_speed * fullTime);
	const float delta			 = float(currentAbscisse - _previousAbscisse);
	_previousAbscisse			 = currentAbscisse;

	const glm::vec3 trans = delta * _amplitude * _axis;
	const glm::mat4 t	 = glm::translate(glm::mat4(1.0f), trans);
	return (_frame == Frame::WORLD ? t * m : m * t);
}

glm::vec4 BackAndForth::apply(const glm::vec4 & v, double fullTime, double) {
	const double currentAbscisse = std::sin(_speed * fullTime);
	const float delta			 = float(currentAbscisse - _previousAbscisse);
	_previousAbscisse			 = currentAbscisse;

	const glm::vec3 trans = delta * _amplitude * _axis;
	return v + glm::vec4(trans, 0.0f);
}

bool BackAndForth::decode(const KeyValues & params) {
	bool success = Animation::decodeBase(params);
	// Decode axis.
	const glm::vec3 newAxis = Codable::decodeVec3(params, 2);
	if(newAxis == glm::vec3(0.0f)){
		return false;
	}
	_axis = glm::normalize(newAxis);
	// Other parameters.
	if(params.values.size() >= 6) {
		_amplitude = std::stof(params.values[5]);
	}
	return success;
}

KeyValues BackAndForth::encode() const {
	KeyValues base = Animation::encode();
	base.key = "backandforth";
	const auto axis = Codable::encode(_axis);
	base.values.insert(base.values.end(), axis.begin(), axis.end());
	base.values.emplace_back(std::to_string(_amplitude));
	return base;
}

