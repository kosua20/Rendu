#include "scene/LightProbe.hpp"
#include "system/TextUtilities.hpp"

bool LightProbe::decode(const KeyValues & params, Storage options) {
	// Assume a static probe initially.
	_type = LightProbe::Type::STATIC;
	std::vector<glm::vec4> coeffs(9, glm::vec4(0.0f));

	bool setCenter = false;
	bool success = true;
	for(const auto & param : params.elements){

		if(param.key == "position"){
			_type = LightProbe::Type::DYNAMIC;
			_position = Codable::decodeVec3(param);

		} else if(param.key == "size"){
			_size = Codable::decodeVec3(param);

		}  else if(param.key == "fade"){
			_fade = std::stof(param.values[0]);

		} else if(param.key == "center"){
			_center = Codable::decodeVec3(param);
			setCenter = true;

		} else if(param.key == "extent"){
			_extent = Codable::decodeVec3(param);

		} else if(param.key == "rotation" && !param.values.empty()){
			_rotation = std::stof(param.values[0]);
			_rotCosSin = glm::vec2(std::cos(_rotation), std::sin(_rotation));

		} else if(param.key == "irradiance" && !param.values.empty()){
			// Load the SH coefficients from the corresponding text file.
			std::string coeffsRaw = Resources::manager().getString(param.values[0]);
			TextUtilities::replace(coeffsRaw, "\n\r", ' ');
			const std::vector<std::string> coeffsTokens = TextUtilities::split(coeffsRaw, " ", true);
			if(coeffsTokens.size() >= 3 * 9){
				for(int i = 0; i < 9; ++i){
					for(int c = 0; c < 3; ++c){
						coeffs[i][c] = std::stof(coeffsTokens[3 * i + c]);
					}
					coeffs[i][3] = 1.f;
				}
			} else {
				success = false;
			}
		} else if(param.key == "radiance" && !param.elements.empty()){
			// Load cubemap described as sub-element.
			const auto texInfos = Codable::decodeTexture(param.elements[0]);
			_envmap = Resources::manager().getTexture(texInfos.first, texInfos.second, options);
		} else {
			Codable::unknown(param);
		}
	}

	if(!setCenter){
		_center = _position;
	}

	// for the static case, check that everything has been provided.
	if(_type == LightProbe::Type::STATIC){
		if(_envmap == nullptr){
			Log::Error() << Log::Resources << "Unable to find envmap for static probe." << std::endl;
			_envmap = Resources::manager().getTexture("default_cube", Layout::RGBA8, options);
			return false;
		}

		if(options & Storage::GPU){
			_shCoeffs.reset(new Buffer(9 * sizeof(glm::vec4), BufferType::STORAGE));
			_shCoeffs->upload(coeffs);
		}
	}
	return success;
}

KeyValues LightProbe::encode() const {
	KeyValues probe("probe");
	if(_type == Type::DYNAMIC){
		probe.elements.emplace_back("position");
		probe.elements.back().values = Codable::encode(_position);
		probe.elements.emplace_back("size");
		probe.elements.back().values = Codable::encode(_size);
		probe.elements.emplace_back("fade");
		probe.elements.back().values.push_back(std::to_string(_fade));
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

void LightProbe::registerEnvironment(const Texture * envmap, const std::shared_ptr<Buffer> & shCoeffs){
	_envmap = envmap;
	_shCoeffs = shCoeffs;
}

void LightProbe::updateSize(const BoundingBox& _bbox){
	const std::vector<glm::vec3> bboxCorners = _bbox.getCorners();
	// Distance from the center of the probe to each corner, per-axis.
	glm::vec3 maxDist(0.0f);
	for(uint i = 0; i < 8; ++i){
		maxDist = glm::max(maxDist, glm::abs(bboxCorners[i] - _position));
	}
	// Use the max distance to a corner as half size, and pad a bit.
	_size = glm::min(_size, maxDist + 0.5f);
}
