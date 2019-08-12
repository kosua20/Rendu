
#include "GameRenderer.hpp"
#include "Common.hpp"

GameRenderer::GameRenderer(RenderingConfig & config) : Renderer(config){
	
	_playerCamera.pose(glm::vec3(0.0f, -5.0f, 24.0f), glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	_playerCamera.projection(config.screenResolution[0]/config.screenResolution[1], 0.6f, 1.0f, 30.0f);
	
	// GL options
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	const int renderWidth = (int)_renderResolution[0];
	const int renderHeight = (int)_renderResolution[1];
	_sceneFramebuffer = std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, {{GL_RGB16F, GL_NEAREST_MIPMAP_NEAREST, GL_CLAMP_TO_EDGE}, {GL_R8, GL_NEAREST_MIPMAP_NEAREST, GL_CLAMP_TO_EDGE}, {GL_DEPTH_COMPONENT32F, GL_NEAREST_MIPMAP_NEAREST, GL_CLAMP_TO_EDGE}}, true));
	_lightingFramebuffer = std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, GL_RGB8, false));
	_fxaaFramebuffer = std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, GL_RGBA8, false));
	
	_fxaaProgram = Resources::manager().getProgram2D("fxaa");
	_finalProgram = Resources::manager().getProgram2D("final_screenquad");
	_compositingProgram = Resources::manager().getProgram2D("game_composite");
	
	_ssaoPass = std::unique_ptr<SSAO>(new SSAO(renderWidth/2, renderHeight/2, 1.5f));
	
	_coloredProgram = Resources::manager().getProgram("colored_object");
	_ground = Resources::manager().getMesh("ground", GPU);
	_head = Resources::manager().getMesh("head", GPU);
	_bodyElement = Resources::manager().getMesh("body", GPU);
	_cubemap = Resources::manager().getCubemap("env", {GL_RGB8, GL_LINEAR_MIPMAP_LINEAR, GL_CLAMP_TO_EDGE});
	
	checkGLError();
}

void GameRenderer::draw(const Player & player){
	
	const glm::vec2 invRenderSize = 1.0f/_renderResolution;
	
	// --- Scene pass ------
	_sceneFramebuffer->bind();
	_sceneFramebuffer->setViewport();
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glEnable(GL_DEPTH_TEST);
	glClear(GL_DEPTH_BUFFER_BIT);
	drawScene(player);
	_sceneFramebuffer->unbind();
	glDisable(GL_DEPTH_TEST);
	
	// --- SSAO pass ------
	_ssaoPass->process(_playerCamera.projection(), _sceneFramebuffer->depthId(), _sceneFramebuffer->textureId(0));
	
	// --- Lighting pass ------
	_lightingFramebuffer->bind();
	_lightingFramebuffer->setViewport();
	glUseProgram(_compositingProgram->id());
	glActiveTexture(GL_TEXTURE0 + 3);
	glBindTexture(GL_TEXTURE_CUBE_MAP, _cubemap->gpu->id);
	ScreenQuad::draw({_sceneFramebuffer->textureId(0), _sceneFramebuffer->textureId(1), _ssaoPass->textureId()});
	_lightingFramebuffer->unbind();
	
	// --- FXAA pass -------
	_fxaaFramebuffer->bind();
	_fxaaFramebuffer->setViewport();
	glUseProgram(_fxaaProgram->id());
	glUniform2fv(_fxaaProgram->uniform("inverseScreenSize"), 1, &(invRenderSize[0]));
	ScreenQuad::draw(_lightingFramebuffer->textureId());
	_fxaaFramebuffer->unbind();
	
	// --- Final pass -------
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, GLsizei(_config.screenResolution[0]), GLsizei(_config.screenResolution[1]));
	glEnable(GL_FRAMEBUFFER_SRGB);
	glUseProgram(_finalProgram->id());
	ScreenQuad::draw(_fxaaFramebuffer->textureId());
	glDisable(GL_FRAMEBUFFER_SRGB);
	checkGLError();
}

void GameRenderer::drawScene(const Player & player){
	// Lighting and reflections will be computed in world space in the shaders.
	// So the normal matrix only takes the model matrix into account.
	
	const glm::mat4 VP = _playerCamera.projection() * _playerCamera.view();
	glUseProgram(_coloredProgram->id());
	// Render the ground.
	{
		const glm::mat4 groundModel = glm::rotate(glm::mat4(1.0f), float(M_PI_2), glm::vec3(1.0f,0.0f,0.0f));
		const glm::mat4 MVP = VP * groundModel;
		const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(groundModel)));
		glUniformMatrix4fv(_coloredProgram->uniform("mvp"), 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix3fv(_coloredProgram->uniform("normalMat"), 1, GL_FALSE, &normalMatrix[0][0]);
		glUniform1i(_coloredProgram->uniform("matID"), 1);
		GLUtilities::drawMesh(*_ground);
	}
	// Render the head.
	{
		const glm::mat4 MVP = VP * player.modelHead;
		const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(player.modelHead)));
		glUniformMatrix4fv(_coloredProgram->uniform("mvp"), 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix3fv(_coloredProgram->uniform("normalMat"), 1, GL_FALSE, &normalMatrix[0][0]);
		glUniform1i(_coloredProgram->uniform("matID"), 2);
		GLUtilities::drawMesh(*_head);
	}
	// Render body elements and items.
	for(int i = 0; i < int(player.modelsBody.size());++i){
		const glm::mat4 MVP = VP * player.modelsBody[i];
		const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(player.modelsBody[i])));
		glUniformMatrix4fv(_coloredProgram->uniform("mvp"), 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix3fv(_coloredProgram->uniform("normalMat"), 1, GL_FALSE, &normalMatrix[0][0]);
		glUniform1i(_coloredProgram->uniform("matID"), player.looksBody[i]);
		GLUtilities::drawMesh(*_bodyElement);
	}
	for(int i = 0; i < int(player.modelsItem.size());++i){
		const glm::mat4 MVP = VP * player.modelsItem[i];
		const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(player.modelsItem[i])));
		glUniformMatrix4fv(_coloredProgram->uniform("mvp"), 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix3fv(_coloredProgram->uniform("normalMat"), 1, GL_FALSE, &normalMatrix[0][0]);
		glUniform1i(_coloredProgram->uniform("matID"), player.looksItem[i]);
		GLUtilities::drawMesh(*_bodyElement);
	}
	// Reset.
	glBindVertexArray(0);
	glUseProgram(0);
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
	_lightingFramebuffer->resize(_renderResolution);
	_ssaoPass->resize((unsigned int)(_renderResolution[0]/2.0f), (unsigned int)(_renderResolution[1]/2.0f));
}

void GameRenderer::clean() {
	Renderer::clean();
	_fxaaFramebuffer->clean();
	_sceneFramebuffer->clean();
	_ssaoPass->clean();
}


void GameRenderer::physics(double , double ){
	
}

GLuint GameRenderer::finalImage() const {
	return _fxaaFramebuffer->textureId();
}

glm::vec2 GameRenderer::renderingResolution() const {
	return _renderResolution;
}

