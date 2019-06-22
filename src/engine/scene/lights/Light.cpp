#include "scene/lights/Light.hpp"

#include "scene/lights/PointLight.hpp"
#include "scene/lights/DirectionalLight.hpp"
#include "scene/lights/SpotLight.hpp"

Light::Light(){
	_castShadows = false;
	_color = glm::vec3(1.0f);
	_mvp = glm::mat4(1.0f);
	_program = nullptr;
	_programDepth = nullptr;
}

Light::Light(const glm::vec3& color){
	_castShadows = false;
	_color = color;
	_mvp = glm::mat4(1.0f);
	_program = nullptr;
	_programDepth = nullptr;
}

void Light::addAnimation(std::shared_ptr<Animation> anim){
	_animations.push_back(anim);
}

void Light::decodeBase(const std::vector<KeyValues> & params){
	for(int pid = 0; pid < params.size(); ++pid){
		const auto & param = params[pid];
		
		if(param.key == "intensity"){
			_color = Codable::decodeVec3(param);
			
		} else if(param.key == "shadows"){
			_castShadows = Codable::decodeBool(param);
			
		} else if(param.key == "animations"){
			_animations = Animation::decode(params, pid);
			--pid;
		}
	}
}

std::shared_ptr<Light> Light::decode(const std::vector<KeyValues> & params){
	if(params.empty()){
		return std::shared_ptr<Light>();
	}
	const std::string typeKey = params[0].key;
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


