#include "Object.hpp"


Object::Object() {}

Object::Object(const Object::Type & type, const std::string& meshPath, const std::vector<std::pair<std::string, bool>>& texturesPaths, const std::vector<std::pair<std::string, bool>>& cubemapPaths, bool castShadows) {

	_material = static_cast<int>(type);
	_castShadow = castShadows;
	
	switch (_material) {
	case Object::Skybox:
		_program = Resources::manager().getProgram("skybox_gbuffer");
		break;
	case Object::Parallax:
		_program = Resources::manager().getProgram("parallax_gbuffer");
		break;
	case Object::Regular:
	default:
		_program = Resources::manager().getProgram("object_gbuffer");
		break;
	}

	// Load geometry.
	_mesh = Resources::manager().getMesh(meshPath);

	// Load and upload the textures.
	for (unsigned int i = 0; i < texturesPaths.size(); ++i) {
		const auto & textureName = texturesPaths[i];
		_textures.push_back(Resources::manager().getTexture(textureName.first, textureName.second));
	}
	for (unsigned int i = 0; i < cubemapPaths.size(); ++i) {
		const auto & textureName = cubemapPaths[i];
		_textures.push_back(Resources::manager().getCubemap(textureName.first, textureName.second));
	}
	
	_model = glm::mat4(1.0f);
	checkGLError();

}


Object::Object(std::shared_ptr<ProgramInfos> & program, const std::string& meshPath, const std::vector<std::pair<std::string, bool>>& texturesPaths, const std::vector<std::pair<std::string, bool>>& cubemapPaths) {
	
	_material = static_cast<int>(Object::Custom);
	_castShadow = false;
	// Load the shaders
	_program = program;
	
	// Load geometry.
	_mesh = Resources::manager().getMesh(meshPath);
	
	// Load and upload the textures.
	for (unsigned int i = 0; i < texturesPaths.size(); ++i) {
		const auto & textureName = texturesPaths[i];
		_textures.push_back(Resources::manager().getTexture(textureName.first, textureName.second));
	}
	for (unsigned int i = 0; i < cubemapPaths.size(); ++i) {
		const auto & textureName = cubemapPaths[i];
		_textures.push_back(Resources::manager().getCubemap(textureName.first, textureName.second));
	}
	_model = glm::mat4(1.0f);
	checkGLError();
	
}

void Object::update(const glm::mat4& model) {

	_model = model;

}


void Object::draw(const glm::mat4& view, const glm::mat4& projection) const {

	// Combine the three matrices.
	glm::mat4 MV = view * _model;
	glm::mat4 MVP = projection * MV;

	// Compute the normal matrix
	glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(MV)));
	// Select the program (and shaders).
	glUseProgram(_program->id());

	// Upload the MVP matrix.
	glUniformMatrix4fv(_program->uniform("mvp"), 1, GL_FALSE, &MVP[0][0]);

	switch (_material) {
		case Object::Parallax:
			// Upload the projection matrix.
			glUniformMatrix4fv(_program->uniform("p"), 1, GL_FALSE, &projection[0][0]);
			// Upload the MV matrix.
			glUniformMatrix4fv(_program->uniform("mv"), 1, GL_FALSE, &MV[0][0]);
			// Upload the normal matrix.
			glUniformMatrix3fv(_program->uniform("normalMatrix"), 1, GL_FALSE, &normalMatrix[0][0]);
			break;
		case Object::Regular:
			// Upload the normal matrix.
			glUniformMatrix3fv(_program->uniform("normalMatrix"), 1, GL_FALSE, &normalMatrix[0][0]);
			break;
		default:
			break;
	}
	

	// Bind the textures.
	for (unsigned int i = 0; i < _textures.size(); ++i){
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(_textures[i].cubemap ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, _textures[i].id);
	}
	drawGeometry();
	glUseProgram(0);
}


void Object::drawGeometry() const {
	glBindVertexArray(_mesh.vId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _mesh.eId);
	glDrawElements(GL_TRIANGLES, _mesh.count, GL_UNSIGNED_INT, (void*)0);
	glBindVertexArray(0);
}


void Object::clean() const {
	glDeleteVertexArrays(1, &_mesh.vId);
	for (auto & texture : _textures) {
		glDeleteTextures(1, &(texture.id));
	}
}

BoundingBox Object::getBoundingBox() const {
	return _mesh.bbox.transformed(_model);
}


