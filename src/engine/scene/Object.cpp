#include "Object.hpp"


Object::Object() {}

Object::Object(const Object::Type type, const MeshInfos * mesh, bool castShadows){
	_material = type;
	_castShadow = castShadows;
	_mesh = mesh;
	_model = glm::mat4(1.0f);
	_textures.clear();
}

void Object::addTexture(const TextureInfos * infos){
	_textures.push_back(infos);
}

void Object::update(const glm::mat4& model) {
	_model = model;
}

BoundingBox Object::getBoundingBox() const {
	return _mesh->bbox.transformed(_model);
}


