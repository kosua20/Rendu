#include "DirectionalLight.hpp"

#include <stdio.h>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include "../helpers/Logger.hpp"




DirectionalLight::DirectionalLight(const glm::vec3& worldDirection, const glm::vec3& color, const BoundingBox & sceneBox) : Light(color) {
	_sceneBox = sceneBox;
	update(worldDirection);
}


void DirectionalLight::init(const std::map<std::string, GLuint>& textureIds){
	// Setup the framebuffer.
	_shadowPass = std::make_shared<Framebuffer>(512, 512, GL_RG,GL_FLOAT, GL_RG16F, GL_LINEAR, GL_CLAMP_TO_BORDER, true);
	_blur = std::make_shared<BoxBlur>(512, 512, false, GL_RG, GL_FLOAT, GL_RG16F, GL_CLAMP_TO_BORDER);
	std::map<std::string, GLuint> textures = textureIds;
	textures["shadowMap"] = _blur->textureId();
	_screenquad.init(textures, "directional_light");
	
}

void DirectionalLight::draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec2& invScreenSize ) const {
	
	glm::mat4 viewToLight = _mvp * glm::inverse(viewMatrix);
	// Store the four variable coefficients of the projection matrix.
	glm::vec4 projectionVector = glm::vec4(projectionMatrix[0][0], projectionMatrix[1][1], projectionMatrix[2][2], projectionMatrix[3][2]);
	glm::vec3 lightDirectionViewSpace = glm::vec3(viewMatrix * glm::vec4(_lightDirection, 0.0));
	
	glUseProgram(_screenquad.program()->id());
	glUniform3fv(_screenquad.program()->uniform("lightDirection"), 1,  &lightDirectionViewSpace[0]);
	glUniform3fv(_screenquad.program()->uniform("lightColor"), 1,  &_color[0]);
	// Projection parameter for position reconstruction.
	glUniform4fv(_screenquad.program()->uniform("projectionMatrix"), 1, &(projectionVector[0]));
	glUniformMatrix4fv(_screenquad.program()->uniform("viewToLight"), 1, GL_FALSE, &viewToLight[0][0]);

	_screenquad.draw();

}

void DirectionalLight::drawShadow(const std::vector<Object> & objects) const {
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
	_blur->process(_shadowPass->textureId());
	glEnable(GL_DEPTH_TEST);
}

void DirectionalLight::drawDebug(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) const {
	
	const std::shared_ptr<ProgramInfos> debugProgram = Resources::manager().getProgram("light_debug", "object_basic", "light_debug");
	const MeshInfos debugMesh = Resources::manager().getMesh("light_arrow");
	
	glm::mat4 vp = projectionMatrix * viewMatrix * glm::inverse(_viewMatrix) * glm::scale(glm::mat4(1.0f), glm::vec3(0.2f));
	const glm::vec3 colorLow = _color/std::max(_color[0], std::max(_color[1], _color[2]));
	
	glUseProgram(debugProgram->id());
	glUniformMatrix4fv(debugProgram->uniform("mvp"), 1, GL_FALSE, &vp[0][0]);
	glUniform3fv(debugProgram->uniform("lightColor"), 1,  &colorLow[0]);
	
	glBindVertexArray(debugMesh.vId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, debugMesh.eId);
	glDrawElements(GL_TRIANGLES, debugMesh.count, GL_UNSIGNED_INT, (void*)0);
	glBindVertexArray(0);
	glUseProgram(0);
}

void DirectionalLight::update(const glm::vec3 & newDirection){
	_lightDirection = glm::normalize(newDirection);
	const BoundingSphere sceneSphere = _sceneBox.getSphere();
	const glm::vec3 lightPosition = sceneSphere.center - sceneSphere.radius*1.1f*_lightDirection;
	const glm::vec3 lightTarget = sceneSphere.center;
	
	_viewMatrix = glm::lookAt(lightPosition, lightTarget, glm::vec3(0.0f,1.0f,0.0f));
	
	const BoundingBox lightSpacebox = _sceneBox.transformed(_viewMatrix);
	const float absz1 = abs(lightSpacebox.minis[2]);
	const float absz2 = abs(lightSpacebox.maxis[2]);
	const float near = std::min(absz1, absz2);
	const float far = std::max(absz1, absz2);
	const float scaleMargin = 1.1f;
	_projectionMatrix = glm::ortho(scaleMargin*lightSpacebox.minis[0], scaleMargin*lightSpacebox.maxis[0], scaleMargin*lightSpacebox.minis[1], scaleMargin*lightSpacebox.maxis[1], (1.0f/scaleMargin)*near, scaleMargin*far);
	_mvp = _projectionMatrix * _viewMatrix;
	
}

void DirectionalLight::clean() const {
	_blur->clean();
	_shadowPass->clean();
}

