#include "scene/LightProbe.hpp"

void LightProbe::decode(const KeyValues & params, Storage options) {
	// Assume a static probe initially.
	_type = LightProbe::Type::STATIC;

	bool setCenter = false;
	for(const auto & param : params.elements){

		if(param.key == "position"){
			_type = LightProbe::Type::DYNAMIC;
			_position = Codable::decodeVec3(param);

		} else if(param.key == "center"){
			_center = Codable::decodeVec3(param);
			setCenter = true;

		} else if(param.key == "extent"){
			_extent = Codable::decodeVec3(param);

		} else if(param.key == "rotation" && !param.values.empty()){
			_rotation = std::stof(param.values[0]);
			_rotCosSin = glm::vec2(std::cos(_rotation), std::sin(_rotation));

		} else if(param.key == "irradiance" && !param.values.empty()){
			_shCoeffs.reset(new UniformBuffer<glm::vec4>(9, UniformFrequency::STATIC));
			// Load the SH coefficients from the corresponding text file.
			const std::string coeffsRaw = Resources::manager().getString(param.values[0]);
			std::stringstream coeffsStream(coeffsRaw);
			float x = 0.0f; float y = 0.0f; float z = 0.0f;
			for(int i = 0; i < 9; ++i) {
				coeffsStream >> x >> y >> z;
				_shCoeffs->at(i) = glm::vec4(x, y, z, 1.0f);
			}
		} else if(param.key == "radiance" && !param.elements.empty()){
			// Load cubemap described as sub-element.
			const auto texInfos = Codable::decodeTexture(param.elements[0]);
			_envmap = Resources::manager().getTexture(texInfos.first, texInfos.second, options);
		}
	}

	if(!setCenter){
		_center = _position;
	}

	// for the static case, check that everything has been provided.
	if(_type == LightProbe::Type::STATIC){
		if(!_envmap){
			_envmap = Resources::manager().getTexture("default_cube", Layout::RGBA8, options);
		}
		if(!_shCoeffs){
			_shCoeffs.reset(new UniformBuffer<glm::vec4>(9, UniformFrequency::STATIC));
			for(int i = 0; i < 9; ++i){
				_shCoeffs->at(i) = glm::vec4(0.0f);
			}
		}
		if(options & Storage::GPU){
			_shCoeffs->upload();
		}
	}
}

KeyValues LightProbe::encode() const {
	KeyValues probe("probe");
	if(_type == Type::DYNAMIC){
		probe.elements.emplace_back("position");
		probe.elements.back().values = Codable::encode(_position);
		probe.elements.emplace_back("extent");
		probe.elements.back().values = Codable::encode(_extent);
		probe.elements.emplace_back("center");
		probe.elements.back().values = Codable::encode(_center);
		probe.elements.emplace_back("rotation");
		probe.elements.back().values.push_back(std::to_string(_rotation));
	} else {
		probe.elements.emplace_back("radiance");
		probe.elements.back().elements.push_back(Codable::encode(_envmap));
		probe.elements.emplace_back("irradiance");
		probe.elements.back().values.push_back("default_shcoeffs");
		Log::Warning() << "Export of static environment data is partially supported for now." << std::endl;
	}
	return probe;
}

void LightProbe::registerEnvironment(const Texture * envmap, const std::shared_ptr<UniformBuffer<glm::vec4>> & shCoeffs){
	_envmap = envmap;
	_shCoeffs = shCoeffs;
}
