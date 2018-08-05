#include "SpotLight.hpp"

#include <stdio.h>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include "../helpers/InterfaceUtilities.hpp"

SpotLight::SpotLight(const glm::vec3& worldPosition, const glm::vec3& worldDirection, const glm::vec3& color, const float innerAngle, const float outerAngle, const float radius, const BoundingBox & sceneBox) : Light(color) {
	
	_innerHalfAngle = 0.5f*innerAngle;
	_outerHalfAngle = 0.5f*outerAngle;
	_radius = radius;
	_sceneBox = sceneBox;
	
	update(worldPosition, worldDirection);

}


void SpotLight::init(const std::map<std::string, GLuint>& textureIds){
	// Setup the framebuffer.
	_shadowPass = std::make_shared<Framebuffer>(512, 512, GL_RG,GL_FLOAT, GL_RG16F, GL_LINEAR, GL_CLAMP_TO_BORDER, true);
	_blurPass = std::make_shared<Framebuffer>(_shadowPass->width(), _shadowPass->height(), GL_RG,GL_FLOAT, GL_RG16F, GL_LINEAR,GL_CLAMP_TO_BORDER, false);
	_blurScreen.init(_shadowPass->textureId(), "box-blur-2");
	
	_program = Resources::manager().getProgram("spot_light", "object_basic", "spot_light");
	_cone = Resources::manager().getMesh("light_cone");
	std::map<std::string, GLuint> textures = textureIds;
	textures["shadowMap"] = _blurPass->textureId();
	
	GLint currentTextureSlot = 0;
	_textureIds.clear();
	for(auto& texture : textures){
		_textureIds.push_back(texture.second);
		_program->registerTexture(texture.first, currentTextureSlot);
		currentTextureSlot += 1;
	}
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
	_shadowPass->bind();
	_shadowPass->setViewport();
	glClearColor(1.0f,1.0f,1.0f,0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	for(auto& object : objects){
		object.drawDepth(_mvp);
	}
	_shadowPass->unbind();
	
	// --- Blur pass --------
	glDisable(GL_DEPTH_TEST);
	_blurPass->bind();
	_blurPass->setViewport();
	_blurScreen.draw();
	_blurPass->unbind();
	glEnable(GL_DEPTH_TEST);
}


void SpotLight::drawDebug(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) const {
	
	const std::shared_ptr<ProgramInfos> debugProgram = Resources::manager().getProgram("light_debug", "object_basic", "light_debug");
	
	// Compute the model matrix to scale the cone based on the outer angle and the radius.
	const float width = 2.0f*std::tan(_outerHalfAngle);
	const glm::mat4 modelMatrix = glm::inverse(_viewMatrix) * glm::scale(glm::mat4(1.0f), _radius*glm::vec3(width,width,1.0f));
	const glm::mat4 mvp = projectionMatrix * viewMatrix * modelMatrix;
	const glm::vec3 colorLow = _color/std::max(_color[0], std::max(_color[1], _color[2]));
	
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
	const float near = std::min(absz1, absz2);
	const float far = std::max(absz1, absz2);
	const float scaleMargin = 1.1f;
	_projectionMatrix = glm::perspective(2.0f*_outerHalfAngle, 1.0f, (1.0f/scaleMargin)*near, scaleMargin*far);
	_mvp = _projectionMatrix * _viewMatrix;
}

void SpotLight::clean() const {
	_blurPass->clean();
	_blurScreen.clean();
	_shadowPass->clean();
}

