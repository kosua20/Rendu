#include "scene/Material.hpp"

#define REGISTER_STRTYPE(type) \
	{ #type, Material::Type::type }
#define REGISTER_TYPESTR(type) \
{ Material::Type::type, #type }


static const std::unordered_map<Material::Type, std::string> typesToStr = {
	REGISTER_TYPESTR(None),
	REGISTER_TYPESTR(Regular),
	REGISTER_TYPESTR(Parallax),
	REGISTER_TYPESTR(Clearcoat),
	REGISTER_TYPESTR(Anisotropic),
	REGISTER_TYPESTR(Sheen),
	REGISTER_TYPESTR(Iridescent),
	REGISTER_TYPESTR(Subsurface),
	REGISTER_TYPESTR(Emissive),
	REGISTER_TYPESTR(Transparent),
	REGISTER_TYPESTR(TransparentIrid),
};

static const std::unordered_map<std::string, Material::Type> strToTypes = {
	REGISTER_STRTYPE(None),
	REGISTER_STRTYPE(Regular),
	REGISTER_STRTYPE(Parallax),
	REGISTER_STRTYPE(Clearcoat),
	REGISTER_STRTYPE(Anisotropic),
	REGISTER_STRTYPE(Sheen),
	REGISTER_STRTYPE(Iridescent),
	REGISTER_STRTYPE(Subsurface),
	REGISTER_STRTYPE(Emissive),
	REGISTER_STRTYPE(Transparent),
	REGISTER_STRTYPE(TransparentIrid),
};

Material::Material(const Type type) :
	_material(type) {
}

bool Material::decode(const KeyValues & params, Storage options) {
	bool success = true;

	for(const auto & param : params.elements) {
		if(param.key == "type" && !param.values.empty()) {
			const std::string typeString = param.values[0];
			if(strToTypes.count(typeString) > 0) {
				_material = strToTypes.at(typeString);
			} else {
				Log::Error() << "Unknown material type: " << typeString << std::endl;
				success = false;
			}

		} else if(param.key == "textures") {
			for(const auto & paramTex : param.elements) {
				const auto texInfos = Codable::decodeTexture(paramTex);
				const Texture * tex = Resources::manager().getTexture(texInfos.first, texInfos.second, options);
				if(tex != nullptr){
					addTexture(tex);
				} else {
					success = false;
				}
			}

		}  else if(param.key == "parameters") {
			for(const auto & paramVec : param.elements) {
				addParameter(Codable::decodeVec4(paramVec));
			}

		} else if(param.key == "twosided") {
			_twoSided = Codable::decodeBool(param);
		} else if(param.key == "masked") {
			_masked = Codable::decodeBool(param);
		} else if(param.key == "name" && !param.values.empty()) {
			_name = param.values[0];
		} else {
			Codable::unknown(param);
		}
	}

	if(_name.empty()){
		Log::Error() << "Material has no name." << std::endl;
		success = false;
	}
	return success;
}

KeyValues Material::encode() const {
	KeyValues obj("material");

	obj.elements.emplace_back("name");
	obj.elements.back().values = {_name};

	obj.elements.emplace_back("type");
	obj.elements.back().values = {typesToStr.at(_material)};

	obj.elements.emplace_back("twosided");
	obj.elements.back().values = {Codable::encode(_twoSided)};
	obj.elements.emplace_back("masked");
	obj.elements.back().values = {Codable::encode(_masked)};
	
	if(!_textures.empty()){
		obj.elements.emplace_back("textures");
		for(const auto texture : _textures){
			if(!texture){
				continue;
			}
			obj.elements.back().elements.push_back(Codable::encode(texture));
		}
	}

	if(!_parameters.empty()){
		obj.elements.emplace_back("parameters");
		KeyValues& params = obj.elements.back();
		uint pid = 0;
		for(const auto& param : _parameters){
			params.elements.emplace_back("p" + std::to_string(pid));
			params.elements.back().values = Codable::encode(param);
			++pid;
		}
	}

	return obj;
}

void Material::addTexture(const Texture * infos) {
	_textures.push_back(infos);
}


void Material::addParameter(const glm::vec4 & param) {
	_parameters.push_back(param);
}
