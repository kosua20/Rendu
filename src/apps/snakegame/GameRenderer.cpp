
#include "GameRenderer.hpp"
#include "graphics/GLUtilities.hpp"
#include "Common.hpp"

GameRenderer::GameRenderer(const glm::vec2 & resolution) {
	_renderResolution = resolution;
	_playerCamera.pose(glm::vec3(0.0f, -5.0f, 24.0f), glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	_playerCamera.projection(_renderResolution[0] / _renderResolution[1], 0.6f, 1.0f, 30.0f);

	// GL options
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	const int renderWidth  = int(_renderResolution[0]);
	const int renderHeight = int(_renderResolution[1]);
	_sceneFramebuffer	  = std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, {{Layout::RGB16F, Filter::NEAREST_NEAREST, Wrap::CLAMP}, {Layout::R8, Filter::NEAREST_NEAREST, Wrap::CLAMP}, {Layout::DEPTH_COMPONENT32F, Filter::NEAREST_NEAREST, Wrap::CLAMP}},
		 true));
	_lightingFramebuffer   = std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, {Layout::RGB8, Filter::LINEAR_NEAREST, Wrap::CLAMP}, false));
	_fxaaFramebuffer	   = std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, {Layout::RGBA8,Filter::LINEAR_NEAREST, Wrap::CLAMP}, false));

	_fxaaProgram		= Resources::manager().getProgram2D("fxaa");
	_compositingProgram = Resources::manager().getProgram2D("game_composite");

	_ssaoPass = std::unique_ptr<SSAO>(new SSAO(renderWidth / 2, renderHeight / 2, 1.5f));

	_coloredProgram = Resources::manager().getProgram("colored_object");
	_ground			= Resources::manager().getMesh("ground", Storage::GPU);
	_head			= Resources::manager().getMesh("head", Storage::GPU);
	_bodyElement	= Resources::manager().getMesh("body", Storage::GPU);
	_cubemap		= Resources::manager().getTexture("env", {Layout::RGB8, Filter::LINEAR_LINEAR, Wrap::CLAMP}, Storage::GPU);

	_renderResult = _fxaaFramebuffer->textureId();
	checkGLError();
}

void GameRenderer::drawPlayer(const Player & player) const {

	const glm::vec2 invRenderSize = 1.0f / _renderResolution;

	// --- Scene pass ------
	_sceneFramebuffer->bind();
	_sceneFramebuffer->setViewport();
	GLUtilities::clearColorAndDepth(glm::vec4(0.0f), 1.0f);
	glEnable(GL_DEPTH_TEST);
	drawScene(player);
	_sceneFramebuffer->unbind();
	glDisable(GL_DEPTH_TEST);

	// --- SSAO pass ------
	_ssaoPass->process(_playerCamera.projection(), _sceneFramebuffer->depthId(), _sceneFramebuffer->textureId(0));

	// --- Lighting pass ------
	_lightingFramebuffer->bind();
	_lightingFramebuffer->setViewport();
	_compositingProgram->use();
	ScreenQuad::draw({_sceneFramebuffer->textureId(0), _sceneFramebuffer->textureId(1), _ssaoPass->textureId(), _cubemap});
	_lightingFramebuffer->unbind();

	// --- FXAA pass -------
	_fxaaFramebuffer->bind();
	_fxaaFramebuffer->setViewport();
	_fxaaProgram->use();
	_fxaaProgram->uniform("inverseScreenSize", invRenderSize);
	ScreenQuad::draw(_lightingFramebuffer->textureId());
	_fxaaFramebuffer->unbind();

}

void GameRenderer::drawScene(const Player & player) const {
	// Lighting and reflections will be computed in world space in the shaders.
	// So the normal matrix only takes the model matrix into account.

	const glm::mat4 VP = _playerCamera.projection() * _playerCamera.view();
	_coloredProgram->use();
	// Render the ground.
	{
		const glm::mat4 groundModel  = glm::rotate(glm::mat4(1.0f), glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
		const glm::mat4 MVP			 = VP * groundModel;
		const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(groundModel)));
		_coloredProgram->uniform("mvp", MVP);
		_coloredProgram->uniform("normalMat", normalMatrix);
		_coloredProgram->uniform("matID", 1);
		GLUtilities::drawMesh(*_ground);
	}
	// Render the head.
	{
		const glm::mat4 MVP			 = VP * player.modelHead;
		const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(player.modelHead)));
		_coloredProgram->uniform("mvp", MVP);
		_coloredProgram->uniform("normalMat", normalMatrix);
		_coloredProgram->uniform("matID", 2);
		GLUtilities::drawMesh(*_head);
	}
	// Render body elements and items.
	for(int i = 0; i < int(player.modelsBody.size()); ++i) {
		const glm::mat4 MVP			 = VP * player.modelsBody[i];
		const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(player.modelsBody[i])));
		_coloredProgram->uniform("mvp", MVP);
		_coloredProgram->uniform("normalMat", normalMatrix);
		_coloredProgram->uniform("matID", player.looksBody[i]);
		GLUtilities::drawMesh(*_bodyElement);
	}
	for(int i = 0; i < int(player.modelsItem.size()); ++i) {
		const glm::mat4 MVP			 = VP * player.modelsItem[i];
		const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(player.modelsItem[i])));
		_coloredProgram->uniform("mvp", MVP);
		_coloredProgram->uniform("normalMat", normalMatrix);
		_coloredProgram->uniform("matID", player.looksItem[i]);
		GLUtilities::drawMesh(*_bodyElement);
	}
}

void GameRenderer::resize(unsigned int width, unsigned int height) {
	_renderResolution = {float(width), float(height)};
	const float aspectRatio = _renderResolution[0] / _renderResolution[1];
	_playerCamera.ratio(aspectRatio);
	_fxaaFramebuffer->resize(_renderResolution);
	_sceneFramebuffer->resize(_renderResolution);
	_lightingFramebuffer->resize(_renderResolution);
	_ssaoPass->resize(uint(_renderResolution[0] / 2.0f), uint(_renderResolution[1] / 2.0f));
}

void GameRenderer::clean() {
	_fxaaFramebuffer->clean();
	_sceneFramebuffer->clean();
	_ssaoPass->clean();
}

glm::vec2 GameRenderer::renderingResolution() const {
	return _renderResolution;
}
