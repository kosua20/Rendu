#include "scene/lights/Light.hpp"

#include "scene/lights/PointLight.hpp"
#include "scene/lights/DirectionalLight.hpp"
#include "scene/lights/SpotLight.hpp"

Light::Light() :
	_program(nullptr), _programDepth(nullptr), _mvp(glm::mat4(1.0f)), _color(glm::vec3(1.0f)), _castShadows(false) {
}

Light::Light(const glm::vec3& color) :
	_program(nullptr), _programDepth(nullptr), _mvp(glm::mat4(1.0f)), _color(color), _castShadows(false) {
}

void Light::addAnimation(const std::shared_ptr<Animation> & anim){
	_animations.push_back(anim);
}

void Light::decodeBase(const KeyValues & params){
	for(const auto & param : params.elements){
		if(param.key == "intensity"){
			_color = Codable::decodeVec3(param);
			
		} else if(param.key == "shadows"){
			_castShadows = Codable::decodeBool(param);
			
		} else if(param.key == "animations"){
			_animations = Animation::decode(param.elements);
			
		}
	}
}

std::shared_ptr<Light> Light::decode(const KeyValues & params){
	
	const std::string typeKey = params.key;
	if(typeKey == "point"){
		auto light = std::shared_ptr<PointLight>(new PointLight());
		light->decode(params);
		return light;
	}
	if(typeKey == "spot"){
		auto light = std::shared_ptr<SpotLight>(new SpotLight());
		light->decode(params);
		return light;
	}
	if(typeKey == "directional"){
		auto light = std::shared_ptr<DirectionalLight>(new DirectionalLight());
		light->decode(params);
		return light;
	}
	return std::shared_ptr<Light>();
}


