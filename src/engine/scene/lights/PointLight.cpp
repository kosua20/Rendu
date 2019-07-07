#include "PointLight.hpp"
#include "Common.hpp"


PointLight::PointLight(): Light() {
	_radius = 1.0f;
	_lightPosition = glm::vec3(0.0f);
}

PointLight::PointLight(const glm::vec3& worldPosition, const glm::vec3& color, float radius) : Light(color) {
	_radius = radius;
	_lightPosition = worldPosition;
}

void PointLight::init(const std::vector<GLuint>& textureIds){
	_program = Resources::manager().getProgram("point_light", "object_basic", "point_light");
	_sphere = Resources::manager().getMesh("light_sphere");
	// Setup the framebuffer.
	/// \todo Enable only if the light is a shadow caster, to avoid wasteful allocation.
	const Descriptor descriptor = {GL_RG16F, GL_LINEAR, GL_CLAMP_TO_EDGE};
	_shadowFramebuffer = std::unique_ptr<FramebufferCube>(new FramebufferCube(512, descriptor, true));
	
	_textures = textureIds;
	_textures.emplace_back(_shadowFramebuffer->textureId());
	// Load the shaders
	_programDepth = Resources::manager().getProgram("object_layer_depth", "object_layer", "light_shadow_linear", "object_layer");
	checkGLError();
}


void PointLight::draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec2& invScreenSize ) const {
	
	// Store the four variable coefficients of the projection matrix.
	const glm::vec4 projectionVector = glm::vec4(projectionMatrix[0][0], projectionMatrix[1][1], projectionMatrix[2][2], projectionMatrix[3][2]);
	const glm::vec3 lightPositionViewSpace = glm::vec3(viewMatrix * glm::vec4(_lightPosition, 1.0f));
	// Compute the model matrix to scale the sphere based on the radius.
	const glm::mat4 modelMatrix = glm::scale(glm::translate(glm::mat4(1.0f), _lightPosition), glm::vec3(_radius));
	const glm::mat4 mvp = projectionMatrix * viewMatrix * modelMatrix;
	const glm::mat3 viewToLight = glm::mat3(glm::inverse(viewMatrix));
	
	glCullFace(GL_FRONT);
	glUseProgram(_program->id());
	glUniformMatrix4fv(_program->uniform("mvp"), 1, GL_FALSE, &mvp[0][0]);
	glUniform3fv(_program->uniform("lightPosition"), 1,  &lightPositionViewSpace[0]);
	glUniform3fv(_program->uniform("lightColor"), 1,  &_color[0]);
	glUniform1f(_program->uniform("lightRadius"), _radius);
	// Projection parameter for position reconstruction.
	glUniform4fv(_program->uniform("projectionMatrix"), 1, &(projectionVector[0]));
	// Inverse screen size uniform.
	glUniform2fv(_program->uniform("inverseScreenSize"), 1, &(invScreenSize[0]));
	glUniformMatrix3fv(_program->uniform("viewToLight"), 1, GL_FALSE, &viewToLight[0][0]);
	glUniform1f(_program->uniform("lightFarPlane"), _farPlane);
	glUniform1i(_program->uniform("castShadow"), _castShadows);
	
	// Active screen texture.
	for(GLuint i = 0;i < _textures.size()-1; ++i){
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, _textures[i]);
	}
	// Activate the shadow cubemap.
	if(_castShadows){
		glActiveTexture(GLenum(GL_TEXTURE0 + _textures.size()-1));
		glBindTexture(GL_TEXTURE_CUBE_MAP, _textures[_textures.size()-1]);
	}
	// Select the geometry.
	GLUtilities::drawMesh(*_sphere);
	
	glBindVertexArray(0);
	glUseProgram(0);
	glCullFace(GL_BACK);
}

void PointLight::drawShadow(const std::vector<Object> & objects) const {
	if(!_castShadows){
		return;
	}
	
	static const char* uniformNames[6] = {"vps[0]", "vps[1]", "vps[2]", "vps[3]", "vps[4]", "vps[5]"};
	
	_shadowFramebuffer->bind();
	_shadowFramebuffer->setViewport();
	glClearColor(1.0f,1.0f,1.0f,1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glUseProgram(_programDepth->id());
	// Udpate the light mvp matrices.
	for(size_t mid = 0; mid < 6; ++mid){
		glUniformMatrix4fv(_programDepth->uniform(uniformNames[mid]), 1, GL_FALSE, &_mvps[mid][0][0]);
	}
	// Pass the world space light position, and the projection matrix far plane.
	glUniform3fv(_programDepth->uniform("lightPositionWorld"), 1, &_lightPosition[0]);
	glUniform1f(_programDepth->uniform("lightFarPlane"), _farPlane);
	
	for(auto& object : objects){
		if(!object.castsShadow()){
			continue;
		}
		glUniformMatrix4fv(_programDepth->uniform("model"), 1, GL_FALSE, &(object.model()[0][0]));
		GLUtilities::drawMesh(*(object.mesh()));
	}
	glUseProgram(0);
	
	_shadowFramebuffer->unbind();
	
	// No blurring pass for now.
}

void PointLight::drawDebug(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) const {
	
	const ProgramInfos * debugProgram = Resources::manager().getProgram("light_debug", "object_basic", "light_debug");
	
	// Compute the model matrix to scale the sphere based on the radius.
	const glm::mat4 modelMatrix = glm::scale(glm::translate(glm::mat4(1.0f), _lightPosition), glm::vec3(_radius));
	const glm::mat4 mvp = projectionMatrix * viewMatrix * modelMatrix;
	const glm::vec3 colorLow = _color/(std::max)(_color[0], (std::max)(_color[1], _color[2]));
	
	glUseProgram(debugProgram->id());
	glUniformMatrix4fv(debugProgram->uniform("mvp"), 1, GL_FALSE, &mvp[0][0]);
	glUniform3fv(debugProgram->uniform("lightColor"), 1,  &colorLow[0]);
	GLUtilities::drawMesh(*_sphere);
	
	const glm::mat4 modelMatrix1 = glm::scale(glm::translate(glm::mat4(1.0f), _lightPosition), glm::vec3(0.02f*_radius));
	const glm::mat4 mvp1 = projectionMatrix * viewMatrix * modelMatrix1;
	glUniformMatrix4fv(debugProgram->uniform("mvp"), 1, GL_FALSE, &mvp1[0][0]);
	glUniform3fv(debugProgram->uniform("lightColor"), 1,  &_color[0]);
	GLUtilities::drawMesh(*_sphere);
	
	glBindVertexArray(0);
	glUseProgram(0);

}

void PointLight::update(double fullTime, double frameTime){
	glm::vec4 position = glm::vec4(_lightPosition, 0.0);
	for(auto & anim : _animations){
		position = anim->apply(position, fullTime, frameTime);
	}
	_lightPosition = glm::vec3(position);
	setScene(_sceneBox);
}

void PointLight::setScene(const BoundingBox & sceneBox){
	_sceneBox = sceneBox;
	
	const glm::mat4 model = glm::translate(glm::mat4(1.0f), -_lightPosition);
	
	// Compute the projection matrix based on the scene bounding box.
	// As both the view matrices and the bounding boxe are axis aligned, we can avoid costly transformations.
	const glm::vec3 deltaMini = _lightPosition - _sceneBox.minis;
	const glm::vec3 deltaMaxi =  _lightPosition - _sceneBox.maxis;
	// Absolute value of each min/max  distance on each axis.
	const glm::vec3 candidatesNear = glm::min(glm::abs(deltaMini), glm::abs(deltaMaxi));
	const glm::vec3 candidatesFar = glm::max(glm::abs(deltaMini), glm::abs(deltaMaxi));
	
	const float size = glm::length(_sceneBox.getSize());
	float far = candidatesFar[0];
	float near = candidatesNear[0];
	bool allInside = true;
	for(int i = 0; i < 3; ++i){
		// The light is inside the bbox along the axis i if the two delta have different signs.
		const bool isInside = (std::signbit(deltaMini[i]) != std::signbit(deltaMaxi[i]));
		allInside = allInside && isInside;
		// In this case we enforce a small near.
		near = (std::min)(near, candidatesNear[i]);
		far = (std::max)(far, candidatesFar[i]);
	}
	if(allInside){
		near = 0.01f*size;
		far = size;
	}
	_farPlane = far;
	const glm::mat4 projection = glm::perspective(float(M_PI/2.0), 1.0f, near, _farPlane);
	
	// Create the constant view matrices for the 6 faces.
	const glm::vec3 ups[6] = { glm::vec3(0.0,-1.0,0.0), glm::vec3(0.0,-1.0,0.0), glm::vec3(0.0,0.0,1.0), glm::vec3(0.0,0.0,-1.0), glm::vec3(0.0,-1.0,0.0), glm::vec3(0.0,-1.0,0.0) };
	const glm::vec3 centers[6] = { glm::vec3(1.0,0.0,0.0), glm::vec3(-1.0,0.0,0.0), glm::vec3(0.0,1.0,0.0), glm::vec3(0.0,-1.0,0.0) , glm::vec3(0.0,0.0,1.0), glm::vec3(0.0,0.0,-1.0)};
	
	// Udpate the mvps.
	_mvps.resize(6);
	for(size_t mid = 0; mid < 6; ++mid){
		const glm::mat4 view = glm::lookAt(glm::vec3(0.0f), centers[mid], ups[mid]);
		_mvps[mid] = projection * view * model;
	}
}

bool PointLight::visible(const glm::vec3 & position, const Raycaster & raycaster, glm::vec3 & direction, float & attenuation) const {
	if(_castShadows && !raycaster.visible(position, _lightPosition)){
		return false;
	}
	direction = _lightPosition - position;
	
	// Early exit if we are outside the sphere of influence.
	const float localRadius = glm::length(direction);
	if(localRadius > _radius){
		return false;
	}
	if(localRadius > 0.0f){
		direction /= localRadius;
	}
	
	// Attenuation with increasing distance to the light.
	const float radiusRatio = localRadius / _radius;
	const float radiusRatio2 = radiusRatio * radiusRatio;
	const float attenNum = glm::clamp(1.0f - radiusRatio2, 0.0f, 1.0f);
	attenuation = attenNum*attenNum;
	return true;
}

void PointLight::decode(const std::vector<KeyValues> & params){
	Light::decodeBase(params);
	for(const auto & param : params){
		if(param.key == "position"){
			_lightPosition = Codable::decodeVec3(param);
		} else if(param.key == "radius" && !param.values.empty()){
			_radius = std::stof(param.values[0]);
		}
	}
}

void PointLight::clean() const {
	_shadowFramebuffer->clean();
}
