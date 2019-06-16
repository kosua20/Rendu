#include "Object.hpp"


Object::Object() {}

Object::Object(const Object::Type type, const MeshInfos * mesh, bool castShadows){
	_material = type;
	_castShadow = castShadows;
	_mesh = mesh;
	_model = glm::mat4(1.0f);
	_textures.clear();
	_animations.clear();
}

void Object::addTexture(const TextureInfos * infos){
	_textures.push_back(infos);
}

void Object::addAnimation(std::shared_ptr<Animation> anim){
	_animations.push_back(anim);
}

void Object::update(double fullTime, double frameTime) {
	glm::mat4 model = _model;
	for(auto anim : _animations){
		model = anim->apply(model, fullTime, frameTime);
	}
	_model = model;
}

BoundingBox Object::getBoundingBox() const {
	return _mesh->bbox.transformed(_model);
}


