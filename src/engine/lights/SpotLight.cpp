#include "SpotLight.hpp"

#include "../helpers/InterfaceUtilities.hpp"

SpotLight::SpotLight(const glm::vec3& worldPosition, const glm::vec3& worldDirection, const glm::vec3& color, const float innerAngle, const float outerAngle, const float radius, const BoundingBox & sceneBox) : Light(color) {
	
	_innerHalfAngle = 0.5f*innerAngle;
	_outerHalfAngle = 0.5f*outerAngle;
	_radius = radius;
	_sceneBox = sceneBox;
	
	update(worldPosition, worldDirection);

}


void SpotLight::init(const std::vector<GLuint>& textureIds){
	// Setup the framebuffer.
	const Framebuffer::Descriptor descriptor = {GL_RG16F, GL_LINEAR, GL_CLAMP_TO_BORDER};
	_shadowPass = std::make_shared<Framebuffer>(512, 512, descriptor, true);
	_blur = std::make_shared<BoxBlur>(512, 512, false, descriptor);
	
	_cone = Resources::manager().getMesh("light_cone");
	_textureIds = textureIds;
	_textureIds.emplace_back(_blur->textureId());
	
	// Load the shaders.
	_program = Resources::manager().getProgram("spot_light", "object_basic", "spot_light");
	_programDepth = Resources::manager().getProgram("object_depth", "object_basic", "light_shadow");
	checkGLError();
}

void SpotLight::draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec2& invScreenSize ) const {
	
	// Store the four variable coefficients of the projection matrix.
	const glm::vec4 projectionVector = glm::vec4(projectionMatrix[0][0], projectionMatrix[1][1], projectionMatrix[2][2], projectionMatrix[3][2]);
	const glm::vec3 lightPositionViewSpace = glm::vec3(viewMatrix * glm::vec4(_lightPosition, 1.0f));
	const glm::vec3 lightDirectionViewSpace = glm::vec3(viewMatrix * glm::vec4(_lightDirection, 0.0f));
	
	// Compute the model matrix to scale the cone based on the outer angle and the radius.
	const float width = 2.0f*std::tan(_outerHalfAngle);
	const glm::mat4 modelMatrix = glm::inverse(_viewMatrix) * glm::scale(glm::mat4(1.0f), _radius*glm::vec3(width,width,1.0f));
	const glm::mat4 mvp = projectionMatrix * viewMatrix * modelMatrix;
	const glm::mat4 viewToLight = _mvp * glm::inverse(viewMatrix);
	
	glUseProgram(_program->id());
	glUniformMatrix4fv(_program->uniform("mvp"), 1, GL_FALSE, &mvp[0][0]);
	glUniform3fv(_program->uniform("lightPosition"), 1,  &lightPositionViewSpace[0]);
	glUniform3fv(_program->uniform("lightDirection"), 1,  &lightDirectionViewSpace[0]);
	glUniform3fv(_program->uniform("lightColor"), 1,  &_color[0]);
	glUniform1f(_program->uniform("lightRadius"), _radius);
	glUniform1f(_program->uniform("innerAngleCos"), std::cos(_innerHalfAngle));
	glUniform1f(_program->uniform("outerAngleCos"), std::cos(_outerHalfAngle));
	// Projection parameter for position reconstruction.
	glUniform4fv(_program->uniform("projectionMatrix"), 1, &(projectionVector[0]));
	// Inverse screen size uniform.
	glUniform2fv(_program->uniform("inverseScreenSize"), 1, &(invScreenSize[0]));
	glUniformMatrix4fv(_program->uniform("viewToLight"), 1, GL_FALSE, &viewToLight[0][0]);
	glUniform1i(_program->uniform("castShadow"), _castShadows);
	
	// Active screen texture.
	for(GLuint i = 0;i < _textureIds.size(); ++i){
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, _textureIds[i]);
	}
	
	// Select the geometry.
	glBindVertexArray(_cone.vId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _cone.eId);
	glDrawElements(GL_TRIANGLES, _cone.count, GL_UNSIGNED_INT, (void*)0);
	
	glBindVertexArray(0);
	glUseProgram(0);

}

void SpotLight::drawShadow(const std::vector<Object> & objects) const {
	if(!_castShadows){
		return;
	}
	_shadowPass->bind();
	_shadowPass->setViewport();
	glClearColor(1.0f,1.0f,1.0f,0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glUseProgram(_programDepth->id());
	for(auto& object : objects){
		if(!object.castsShadow()){
			continue;
		}
		const glm::mat4 lightMVP = _mvp * object.model();
		glUniformMatrix4fv(_programDepth->uniform("mvp"), 1, GL_FALSE, &lightMVP[0][0]);
		object.drawGeometry();
	}
	glUseProgram(0);
	
	_shadowPass->unbind();
	
	// --- Blur pass --------
	glDisable(GL_DEPTH_TEST);
	_blur->process(_shadowPass->textureId());
	glEnable(GL_DEPTH_TEST);
}


void SpotLight::drawDebug(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) const {
	
	const std::shared_ptr<ProgramInfos> debugProgram = Resources::manager().getProgram("light_debug", "object_basic", "light_debug");
	
	// Compute the model matrix to scale the cone based on the outer angle and the radius.
	const float width = 2.0f*std::tan(_outerHalfAngle);
	const glm::mat4 modelMatrix = glm::inverse(_viewMatrix) * glm::scale(glm::mat4(1.0f), _radius*glm::vec3(width,width,1.0f));
	const glm::mat4 mvp = projectionMatrix * viewMatrix * modelMatrix;
	const glm::vec3 colorLow = _color/(std::max)(_color[0], (std::max)(_color[1], _color[2]));
	
	glUseProgram(debugProgram->id());
	glUniformMatrix4fv(debugProgram->uniform("mvp"), 1, GL_FALSE, &mvp[0][0]);
	glUniform3fv(debugProgram->uniform("lightColor"), 1,  &colorLow[0]);
	
	glBindVertexArray(_cone.vId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _cone.eId);
	glDrawElements(GL_TRIANGLES, _cone.count, GL_UNSIGNED_INT, (void*)0);
	glBindVertexArray(0);
	glUseProgram(0);
}


void SpotLight::update(const glm::vec3 & newPosition){
	update(newPosition, _lightDirection);
}

void SpotLight::update(const glm::vec3 & newPosition, const glm::vec3 & newDirection){
	_lightPosition = newPosition;
	_lightDirection = glm::normalize(newDirection);
	_viewMatrix = glm::lookAt(_lightPosition, _lightPosition+_lightDirection, glm::vec3(0.0f,1.0f,0.0f));
	// Compute the projection matrix, automatically finding the near and far.
	const BoundingBox lightSpacebox = _sceneBox.transformed(_viewMatrix);
	const float absz1 = abs(lightSpacebox.minis[2]);
	const float absz2 = abs(lightSpacebox.maxis[2]);
	const float near = (std::min)(absz1, absz2);
	const float far = (std::max)(absz1, absz2);
	const float scaleMargin = 1.5f;
	_projectionMatrix = glm::perspective(2.0f*_outerHalfAngle, 1.0f, (1.0f/scaleMargin)*near, scaleMargin*far);
	_mvp = _projectionMatrix * _viewMatrix;
}

void SpotLight::clean() const {
	_blur->clean();
	_shadowPass->clean();
}

