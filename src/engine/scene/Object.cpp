#include "scene/Object.hpp"
#include <map>

#define REGISTER_STRTYPE(type) \
	{ #type, Type::type }
#define REGISTER_TYPESTR(type) \
{ Type::type, #type }


Object::Object(const Type type, const Mesh * mesh, bool castShadows) :
	_mesh(mesh), _material(type), _castShadow(castShadows) {
}

void Object::decode(const KeyValues & params, Storage options) {

	static const std::map<std::string, Object::Type> types = {
		REGISTER_STRTYPE(Common),
		REGISTER_STRTYPE(PBRRegular),
		REGISTER_STRTYPE(PBRParallax),
		REGISTER_STRTYPE(PBRNoUVs)};

	// We expect there is only one transformation in the parameters set.
	_model.reset(Codable::decodeTransformation(params.elements));
	
	for(const auto & param : params.elements) {
		if(param.key == "type" && !param.values.empty()) {
			const std::string typeString = param.values[0];
			if(types.count(typeString) > 0) {
				_material = types.at(typeString);
			}

		} else if(param.key == "mesh" && !param.values.empty()) {
			const std::string meshString = param.values[0];
			_mesh						 = Resources::manager().getMesh(meshString, options);

		} else if(param.key == "shadows") {
			_castShadow = Codable::decodeBool(param);

		} else if(param.key == "textures") {
			for(const auto & paramTex : param.elements) {
				const auto texInfos = Codable::decodeTexture(paramTex);
				const Texture * tex = Resources::manager().getTexture(texInfos.first, texInfos.second, options);
				addTexture(tex);
			}

		} else if(param.key == "animations") {
			_animations = Animation::decode(param.elements);
		} else if(param.key == "twosided") {
			_twoSided = Codable::decodeBool(param);
		} else if(param.key == "masked") {
			_masked = Codable::decodeBool(param);
		}
	}
}

KeyValues Object::encode() const {
	KeyValues obj("object");
	static const std::map<Object::Type, std::string> types = {
		REGISTER_TYPESTR(Common),
		REGISTER_TYPESTR(PBRRegular),
		REGISTER_TYPESTR(PBRParallax),
		REGISTER_TYPESTR(PBRNoUVs)};

	obj.elements.emplace_back("type");
	obj.elements.back().values = {types.at(_material)};
	
	obj.elements.emplace_back("shadows");
	obj.elements.back().values = {Codable::encode(_castShadow)};
	obj.elements.emplace_back("twosided");
	obj.elements.back().values = {Codable::encode(_twoSided)};
	obj.elements.emplace_back("masked");
	obj.elements.back().values = {Codable::encode(_masked)};
	
	if(_mesh){
		obj.elements.emplace_back("mesh");
		obj.elements.back().values = {_mesh->name()};
	}
	
	if(!_animations.empty()){
		obj.elements.emplace_back("animations");
		obj.elements.back().elements = Animation::encode(_animations);
	}
	
	if(!_textures.empty()){
		obj.elements.emplace_back("textures");
		for(const auto texture : _textures){
			if(!texture){
				continue;
			}
			obj.elements.back().elements.push_back(Codable::encode(texture));
		}
	}
	
	const auto transfo = Codable::encode(_model.initial());
	for(const auto & param : transfo){
		obj.elements.push_back(param);
	}
	return obj;
}

void Object::addTexture(const Texture * infos) {
	_textures.push_back(infos);
}

void Object::addAnimation(const std::shared_ptr<Animation> & anim) {
	_animations.push_back(anim);
}


void Object::set(const glm::mat4 & model){
	_model.reset(model);
}

void Object::update(double fullTime, double frameTime) {
	glm::mat4 model = _model;
	for(auto & anim : _animations) {
		model = anim->apply(model, fullTime, frameTime);
	}
	_model = model;
}

BoundingBox Object::boundingBox() const {
	return _mesh->bbox.transformed(_model);
}
