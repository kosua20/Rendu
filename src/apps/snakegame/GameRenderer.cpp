
#include "GameRenderer.hpp"
#include "Common.hpp"
#include "helpers/InterfaceUtilities.hpp"
#include "input/Input.hpp"

GameRenderer::GameRenderer(RenderingConfig & config) : Renderer(config){
	
	_playerCamera.pose(glm::vec3(0.0f, 0.0f, 25.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	_playerCamera.projection(config.screenResolution[0]/config.screenResolution[1], 0.6f, 1.0f, 30.0f);
	
	// GL options
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	const int renderWidth = (int)_renderResolution[0];
	const int renderHeight = (int)_renderResolution[1];
	_sceneFramebuffer = std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, GL_RGBA8, true));
	_fxaaFramebuffer = std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, GL_RGBA8, false));
	_fxaaProgram = Resources::manager().getProgram2D("fxaa");
	_finalProgram = Resources::manager().getProgram2D("final_screenquad");
	
	_coloredProgram = Resources::manager().getProgram("colored_object");
	_head = Resources::manager().getMesh("head");
	_bodyElement = Resources::manager().getMesh("body");
}

void GameRenderer::draw(const Player & player){
	
	const glm::vec2 invRenderSize = 1.0f/_renderResolution;
	
	
	_sceneFramebuffer->bind();
	_sceneFramebuffer->setViewport();
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// Render the ground.
	const auto & groundProgram = _coloredProgram;
	glUseProgram(groundProgram->id());
	const auto mesh  = Resources::manager().getMesh("ground");
	// Upload the MVP matrix.
	const glm::mat4 groundModel = glm::rotate(glm::mat4(1.0f), float(M_PI_2), glm::vec3(1.0f,0.0f,0.0f));
	const glm::mat4 MVP = _playerCamera.projection() * _playerCamera.view() * groundModel;
	const glm::mat3 normalMatrix = glm::inverse(glm::transpose(glm::mat3(groundModel)));
	glUniformMatrix4fv(groundProgram->uniform("mvp"), 1, GL_FALSE, &MVP[0][0]);
	glUniformMatrix3fv(groundProgram->uniform("normalMat"), 1, GL_FALSE, &normalMatrix[0][0]);
	glUniform3f(groundProgram->uniform("baseColor"), 0.5f, 0.4f, 0.2f);
	glBindVertexArray(mesh.vId);
	glDrawElements(GL_TRIANGLES, mesh.count, GL_UNSIGNED_INT, (void*)0);
	glBindVertexArray(0);
	glUseProgram(0);
		
	// Render the items and player.
	glUseProgram(_coloredProgram->id());
	const glm::mat4 VP = _playerCamera.projection() * _playerCamera.view();
	
	{
		
		const glm::mat4 MVP = VP * player.modelHead;
		// \todo If no sheering, can avoid the inverse transpose.
		const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(player.modelHead)));
		
		glUniformMatrix4fv(_coloredProgram->uniform("mvp"), 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix3fv(_coloredProgram->uniform("normalMat"), 1, GL_FALSE, &normalMatrix[0][0]);
		glBindVertexArray(_head.vId);
		glUniform3f(_coloredProgram->uniform("baseColor"), 0.1f, 0.6f, 0.9f);
		glDrawElements(GL_TRIANGLES, _head.count, GL_UNSIGNED_INT, (void*)0);
	}
	
	glBindVertexArray(_bodyElement.vId);
	for(int i = 0; i < player.modelsBody.size();++i){
		
		const glm::mat4 MVP = VP * player.modelsBody[i];
		const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(player.modelsBody[i])));
		glUniformMatrix4fv(_coloredProgram->uniform("mvp"), 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix3fv(_coloredProgram->uniform("normalMat"), 1, GL_FALSE, &normalMatrix[0][0]);
		glUniform3f(_coloredProgram->uniform("baseColor"), 0.1f, 0.9f, 0.2f);
		glDrawElements(GL_TRIANGLES, _bodyElement.count, GL_UNSIGNED_INT, (void*)0);
	}
	
	glBindVertexArray(_bodyElement.vId);
	for(int i = 0; i < player.modelsItem.size();++i){
		const glm::mat4 MVP1 = VP * player.modelsItem[i];
		const glm::mat3 normalMatrix1 = glm::transpose(glm::inverse(glm::mat3(player.modelsItem[i])));
		glUniformMatrix4fv(_coloredProgram->uniform("mvp"), 1, GL_FALSE, &MVP1[0][0]);
		glUniformMatrix3fv(_coloredProgram->uniform("normalMat"), 1, GL_FALSE, &normalMatrix1[0][0]);
		glUniform3f(_coloredProgram->uniform("baseColor"), 0.9f, 0.1f, 0.1f);
		glDrawElements(GL_TRIANGLES, _bodyElement.count, GL_UNSIGNED_INT, (void*)0);
	}
	
	glBindVertexArray(0);
	glUseProgram(0);
	
	
	_sceneFramebuffer->unbind();
	
	glDisable(GL_DEPTH_TEST);
	
	// --- FXAA pass -------
	_fxaaFramebuffer->bind();
	_fxaaFramebuffer->setViewport();
	glUseProgram(_fxaaProgram->id());
	glUniform2fv(_fxaaProgram->uniform("inverseScreenSize"), 1, &(invRenderSize[0]));
	ScreenQuad::draw(_sceneFramebuffer->textureId());
	_fxaaFramebuffer->unbind();
	
	// --- Final pass -------
	glViewport(0, 0, GLsizei(_config.screenResolution[0]), GLsizei(_config.screenResolution[1]));
	glUseProgram(_finalProgram->id());
	ScreenQuad::draw(_fxaaFramebuffer->textureId());
	
	checkGLError();
}

void GameRenderer::update(){
	Renderer::update();
}

void GameRenderer::resize(unsigned int width, unsigned int height){
	Renderer::updateResolution(width, height);
	const float aspectRatio = _renderResolution[0]/_renderResolution[1];
	_playerCamera.ratio(aspectRatio);
	_fxaaFramebuffer->resize(_renderResolution);
	_sceneFramebuffer->resize(_renderResolution);
}

void GameRenderer::clean() const {
	Renderer::clean();
	_fxaaFramebuffer->clean();
	_sceneFramebuffer->clean();
	glDeleteVertexArrays(1, &_head.vId);
	glDeleteVertexArrays(1, &_bodyElement.vId);
}


