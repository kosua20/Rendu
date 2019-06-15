#include "Object.hpp"


Object::Object() {}

Object::Object(const Object::Type & type, const std::string& meshPath, const std::vector<std::pair<std::string, bool>>& texturesPaths, const std::vector<std::pair<std::string, bool>>& cubemapPaths, bool castShadows) {

	_material = type;
	_castShadow = castShadows;
	
	
	// Load geometry.
	_mesh = Resources::manager().getMesh(meshPath);
	
	// Load and upload the textures.
	const Descriptor rgbaTex(GL_RGBA8, GL_LINEAR, GL_CLAMP_TO_EDGE);
	const Descriptor srgbaTex(GL_SRGB8_ALPHA8, GL_LINEAR, GL_CLAMP_TO_EDGE);
	for (unsigned int i = 0; i < texturesPaths.size(); ++i) {
		const auto & textureName = texturesPaths[i];
		_textures.push_back(Resources::manager().getTexture(textureName.first, textureName.second ? srgbaTex : rgbaTex));
	}
	for (unsigned int i = 0; i < cubemapPaths.size(); ++i) {
		const auto & textureName = cubemapPaths[i];
		_textures.push_back(Resources::manager().getCubemap(textureName.first, textureName.second ? srgbaTex : rgbaTex));
	}
	
	_model = glm::mat4(1.0f);
	checkGLError();

}



void Object::update(const glm::mat4& model) {
	_model = model;
}

BoundingBox Object::getBoundingBox() const {
	return _mesh->bbox.transformed(_model);
}


