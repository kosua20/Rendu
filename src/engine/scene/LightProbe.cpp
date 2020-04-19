#include "scene/LightProbe.hpp"

void LightProbe::decode(const KeyValues & params, Storage options) {
	// Two possibilities: a precomputed cubemap or a real-time probe.
	if(!params.elements.empty()){
		_type = Type::STATIC;
		// Load cubemap described as sub-element.
		const auto texInfos = Codable::decodeTexture(params.elements[0]);
		_envmap = Resources::manager().getTexture(texInfos.first, texInfos.second, options);
		_position = glm::vec3(0.0f);
	} else {
		_type = Type::DYNAMIC;
		_position = Codable::decodeVec3(params);
	}
}

KeyValues LightProbe::encode() const {
	KeyValues probe("probe");
	if(_type == Type::DYNAMIC){
		probe.values = {Codable::encode(_position)};
	} else {
		probe.elements = {Codable::encode(_envmap)};
	}
	return probe;
}

void LightProbe::registerEnvmap(const Texture * envmap){
	_envmap = envmap;
}

bool LightProbe::dynamic() const {
	return _type == Type::DYNAMIC;
}

