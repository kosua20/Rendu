#include "scene/Sky.hpp"

Sky::Sky(Storage options) :
	Object(Resources::manager().getMesh("plane", options), false) {
}

bool Sky::decode(const KeyValues & params, Storage options) {
	bool success = Object::decode(params, options);
	for(const auto & param : params.elements) {
		if(param.key == "sun") {
			const glm::vec3 newDir = Codable::decodeVec3(param);
			if(newDir == glm::vec3(0.0f)){
				Log::Info() << "Invalid null sun direction." << std::endl;
				return false;
			}
			_sunDirection.reset(glm::normalize(newDir));
			
		}
	}
	return success;
}

KeyValues Sky::encode() const {
	KeyValues obj = Object::encode();
	obj.elements.emplace_back("sun");
	obj.elements.back().values = Codable::encode(_sunDirection.initial());
	return obj;
}

void Sky::update(double fullTime, double frameTime) {
	glm::vec4 dir = glm::vec4(_sunDirection.get(), 0.0f);
	for(auto & anim : _animations) {
		dir = anim->apply(dir, fullTime, frameTime);
	}
	_sunDirection = glm::normalize(glm::vec3(dir));
}
