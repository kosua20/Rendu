#include "scene/Sky.hpp"


Sky::Sky(const Storage mode) : Object(Object::Type::Common, Resources::manager().getMesh("plane", mode), false){
	
}


void Sky::decode(const std::vector<KeyValues> & params, const Storage mode){
	Object::decode(params, mode);
	for(size_t pid = 0; pid < params.size(); ++pid){
		const auto & param = params[pid];
		if(param.key == "direction"){
			_sunDirection = Codable::decodeVec3(param);
			_sunDirection = glm::normalize(_sunDirection);
		}
	}
}


void Sky::update(double fullTime, double frameTime) {
	glm::vec4 dir = glm::vec4(_sunDirection, 0.0f);
	for(auto anim : _animations){
		dir = anim->apply(dir, fullTime, frameTime);
	}
	_sunDirection = glm::normalize(glm::vec3(dir));
}
