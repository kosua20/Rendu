#include "scene/Sky.hpp"

Sky::Sky(Storage options) :
	Object(Object::Type::Common, Resources::manager().getMesh("plane", options), false) {
}

void Sky::decode(const KeyValues & params, Storage options) {
	Object::decode(params, options);
	for(const auto & param : params.elements) {
		if(param.key == "sun") {
			_sunDirection = glm::normalize(Codable::decodeVec3(param));
		}
	}
}

KeyValues Sky::encode() const {
	KeyValues obj = Object::encode();
	obj.elements.emplace_back("sun");
	obj.elements.back().values = {Codable::encode(_sunDirection.initial())};
	return obj;
}

void Sky::update(double fullTime, double frameTime) {
	glm::vec4 dir = glm::vec4(_sunDirection.get(), 0.0f);
	for(auto & anim : _animations) {
		dir = anim->apply(dir, fullTime, frameTime);
	}
	_sunDirection = glm::normalize(glm::vec3(dir));
}
