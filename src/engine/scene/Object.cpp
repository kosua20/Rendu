#include "scene/Object.hpp"
#include "scene/Material.hpp"

Object::Object(const Mesh * mesh, bool castShadows) :
	_mesh(mesh), _castShadow(castShadows) {
	// Skip UVs if not available.
	_skipUVs = !_mesh->hadTexcoords();
}

void Object::decode(const KeyValues & params, Storage options) {

	// We expect there is only one transformation in the parameters set.
	_model.reset(Codable::decodeTransformation(params.elements));
	
	for(const auto & param : params.elements) {
		if(param.key == "mesh" && !param.values.empty()) {
			const std::string meshString = param.values[0];
			_mesh						 = Resources::manager().getMesh(meshString, options);

		} else if(param.key == "material" && !param.values.empty()) {
			_materialName = param.values[0];

		} else if(param.key == "shadows") {
			_castShadow = Codable::decodeBool(param);

		} else if(param.key == "animations") {
			_animations = Animation::decode(param.elements);
		} else if(param.key == "skipuvs") {
			_skipUVs = Codable::decodeBool(param);
		}
	}

	// If the mesh doesn't have texture coordinates, skip UVs.
	_skipUVs = _skipUVs || (!_mesh->hadTexcoords());
}

KeyValues Object::encode() const {
	KeyValues obj("object");

	obj.elements.emplace_back("material");
	obj.elements.back().values = {_material ? _material->name() : _materialName};

	obj.elements.emplace_back("shadows");
	obj.elements.back().values = {Codable::encode(_castShadow)};
	obj.elements.emplace_back("skipuvs");
	obj.elements.back().values = {Codable::encode(_skipUVs)};
	
	if(_mesh){
		obj.elements.emplace_back("mesh");
		obj.elements.back().values = {_mesh->name()};
	}
	
	if(!_animations.empty()){
		obj.elements.emplace_back("animations");
		obj.elements.back().elements = Animation::encode(_animations);
	}
	
	const auto transfo = Codable::encode(_model.initial());
	for(const auto & param : transfo){
		obj.elements.push_back(param);
	}
	return obj;
}

void Object::setMaterial(const Material* material){
	_material = material;
	_materialName = material->name();
}

void Object::addAnimation(const std::shared_ptr<Animation> & anim) {
	_animations.push_back(anim);
}

void Object::set(const glm::mat4 & model){
	_model.reset(model);
	_dirtyBbox = true;
}

void Object::update(double fullTime, double frameTime) {
	glm::mat4 model = _model;
	for(auto & anim : _animations) {
		model = anim->apply(model, fullTime, frameTime);
	}
	if(!_animations.empty()){
		_dirtyBbox = true;
	}
	_model = model;
}

const BoundingBox & Object::boundingBox() const {
	if(_dirtyBbox){
		_bbox = _mesh->bbox.transformed(_model);
		_dirtyBbox = false;
	}
	return _bbox;
}
