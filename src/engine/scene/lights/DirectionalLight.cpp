#include "scene/lights/DirectionalLight.hpp"
#include "graphics/GLUtilities.hpp"

DirectionalLight::DirectionalLight() : Light() {
	_lightDirection = glm::vec3(1.0f, 0.0f, 0.0f);
}

DirectionalLight::DirectionalLight(const glm::vec3& worldDirection, const glm::vec3& color) : Light(color) {
	_lightDirection = glm::normalize(worldDirection);
}


void DirectionalLight::init(const std::vector<GLuint>& textureIds){
	// Setup the framebuffer.
	const Descriptor descriptor = { GL_RG16F, GL_LINEAR, GL_CLAMP_TO_BORDER };
	_shadowPass = std::unique_ptr<Framebuffer>(new Framebuffer(512, 512, descriptor, true));
	_blur = std::unique_ptr<BoxBlur>(new BoxBlur(512, 512, false, descriptor));
	
	_textures = textureIds;
	_textures.emplace_back(_blur->textureId());
	
	// Load the shaders
	_program = Resources::manager().getProgram2D("directional_light");
	_programDepth = Resources::manager().getProgram("object_depth", "object_basic_texture", "light_shadow");

}

void DirectionalLight::draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec2&) const {
	
	glm::mat4 viewToLight = _mvp * glm::inverse(viewMatrix);
	// Store the four variable coefficients of the projection matrix.
	glm::vec4 projectionVector = glm::vec4(projectionMatrix[0][0], projectionMatrix[1][1], projectionMatrix[2][2], projectionMatrix[3][2]);
	glm::vec3 lightDirectionViewSpace = glm::vec3(viewMatrix * glm::vec4(_lightDirection, 0.0));
	
	glUseProgram(_program->id());
	glUniform3fv(_program->uniform("lightDirection"), 1,  &lightDirectionViewSpace[0]);
	glUniform3fv(_program->uniform("lightColor"), 1,  &_color[0]);
	// Projection parameter for position reconstruction.
	glUniform4fv(_program->uniform("projectionMatrix"), 1, &(projectionVector[0]));
	glUniformMatrix4fv(_program->uniform("viewToLight"), 1, GL_FALSE, &viewToLight[0][0]);
	glUniform1i(_program->uniform("castShadow"), _castShadows);

	ScreenQuad::draw(_textures);

}

void DirectionalLight::drawShadow(const std::vector<Object> & objects) const {
	if(!_castShadows){
		return;
	}
	_shadowPass->bind();
	_shadowPass->setViewport();
	glClearColor(1.0f,1.0f,1.0f,0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_CULL_FACE);
	
	glUseProgram(_programDepth->id());
	for(auto& object : objects){
		if(!object.castsShadow()){
			continue;
		}
		if(object.twoSided()){
			glDisable(GL_CULL_FACE);
		}
		glUniform1i(_programDepth->uniform("hasMask"), int(object.masked()));
		if(object.masked()){
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, object.textures()[0]->gpu->id);
		}
		const glm::mat4 lightMVP = _mvp * object.model();
		glUniformMatrix4fv(_programDepth->uniform("mvp"), 1, GL_FALSE, &lightMVP[0][0]);
		GLUtilities::drawMesh(*(object.mesh()));
		glEnable(GL_CULL_FACE);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	glUseProgram(0);
	
	_shadowPass->unbind();
	
	// --- Blur pass --------
	glDisable(GL_DEPTH_TEST);
	_blur->process(_shadowPass->textureId());
	glEnable(GL_DEPTH_TEST);
}

void DirectionalLight::drawDebug(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) const {
	
	const ProgramInfos * debugProgram = Resources::manager().getProgram("light_debug", "object_basic", "light_debug");
	const Mesh * debugMesh = Resources::manager().getMesh("light_arrow", GPU);
	
	glm::mat4 vp = projectionMatrix * viewMatrix * glm::inverse(_viewMatrix) * glm::scale(glm::mat4(1.0f), glm::vec3(0.2f));
	const glm::vec3 colorLow = _color/(std::max)(_color[0], (std::max)(_color[1], _color[2]));
	
	glUseProgram(debugProgram->id());
	glUniformMatrix4fv(debugProgram->uniform("mvp"), 1, GL_FALSE, &vp[0][0]);
	glUniform3fv(debugProgram->uniform("lightColor"), 1,  &colorLow[0]);
	GLUtilities::drawMesh(*debugMesh);
	glBindVertexArray(0);
	glUseProgram(0);
}

void DirectionalLight::update(double fullTime, double frameTime){
	glm::vec4 direction = glm::vec4(_lightDirection, 0.0f);
	for(auto & anim : _animations){
		direction = anim->apply(direction, fullTime, frameTime);
	}
	_lightDirection = glm::normalize(glm::vec3(direction));
	setScene(_sceneBox);
}

void DirectionalLight::setScene(const BoundingBox & sceneBox){
	_sceneBox = sceneBox;
	const BoundingSphere sceneSphere = _sceneBox.getSphere();
	const glm::vec3 lightPosition = sceneSphere.center - sceneSphere.radius*1.1f*_lightDirection;
	const glm::vec3 lightTarget = sceneSphere.center;
	_viewMatrix = glm::lookAt(lightPosition, lightTarget, glm::vec3(0.0f,1.0f,0.0f));
	
	const BoundingBox lightSpacebox = _sceneBox.transformed(_viewMatrix);
	const float absz1 = abs(lightSpacebox.minis[2]);
	const float absz2 = abs(lightSpacebox.maxis[2]);
	const float near = (std::min)(absz1, absz2);
	const float far = (std::max)(absz1, absz2);
	const float scaleMargin = 1.5f;
	_projectionMatrix = glm::ortho(scaleMargin*lightSpacebox.minis[0], scaleMargin*lightSpacebox.maxis[0], scaleMargin*lightSpacebox.minis[1], scaleMargin*lightSpacebox.maxis[1], (1.0f/scaleMargin)*near, scaleMargin*far);
	_mvp = _projectionMatrix * _viewMatrix;
}

void DirectionalLight::clean() const {
	if(_blur){
		_blur->clean();
	}
	if(_shadowPass){
		_shadowPass->clean();
	}
}

bool DirectionalLight::visible(const glm::vec3 & position, const Raycaster & raycaster, glm::vec3 & direction, float & attenuation) const {
	
	if(_castShadows && raycaster.intersectsAny(position, -_lightDirection)){
		return false;
	}
	direction = -_lightDirection;
	attenuation = 1.0f;
	return true;
}

void DirectionalLight::decode(const KeyValues & params){
	Light::decodeBase(params);
	for(const auto & param : params.elements){
		if(param.key == "direction"){
			_lightDirection = glm::normalize(Codable::decodeVec3(param));
		}
	}
}
