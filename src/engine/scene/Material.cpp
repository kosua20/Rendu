#include "scene/Material.hpp"

#define REGISTER_STRTYPE(type) \
	{ #type, Material::Type::type }
#define REGISTER_TYPESTR(type) \
{ Material::Type::type, #type }


static const std::unordered_map<Material::Type, std::string> typesToStr = {
	REGISTER_TYPESTR(None),
	REGISTER_TYPESTR(Regular),
	REGISTER_TYPESTR(Parallax),
	REGISTER_TYPESTR(Emissive),
	REGISTER_TYPESTR(Transparent),
};

static const std::unordered_map<std::string, Material::Type> strToTypes = {
	REGISTER_STRTYPE(None),
	REGISTER_STRTYPE(Regular),
	REGISTER_STRTYPE(Parallax),
	REGISTER_STRTYPE(Emissive),
	REGISTER_STRTYPE(Transparent),
};

Material::Material(const Type type) :
	_material(type) {
}

void Material::decode(const KeyValues & params, Storage options) {

	for(const auto & param : params.elements) {
		if(param.key == "type" && !param.values.empty()) {
			const std::string typeString = param.values[0];
			if(strToTypes.count(typeString) > 0) {
				_material = strToTypes.at(typeString);
			}

		} else if(param.key == "textures") {
			for(const auto & paramTex : param.elements) {
				const auto texInfos = Codable::decodeTexture(paramTex);
				const Texture * tex = Resources::manager().getTexture(texInfos.first, texInfos.second, options);
				addTexture(tex);
			}

		} else if(param.key == "twosided") {
			_twoSided = Codable::decodeBool(param);
		} else if(param.key == "masked") {
			_masked = Codable::decodeBool(param);
		} else if(param.key == "name" && !param.values.empty()) {
			_name = param.values[0];
		}
	}
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

	return obj;
}

void Material::addTexture(const Texture * infos) {
	_textures.push_back(infos);
}
