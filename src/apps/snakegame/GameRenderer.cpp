
#include "GameRenderer.hpp"
#include "graphics/GPU.hpp"
#include "graphics/ScreenQuad.hpp"
#include "resources/ResourcesManager.hpp"
#include "Common.hpp"

GameRenderer::GameRenderer(const glm::vec2 & resolution) : Renderer("Game") {
	_playerCamera.pose(glm::vec3(0.0f, -5.0f, 24.0f), glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	_playerCamera.projection(resolution[0] / resolution[1], 0.6f, 1.0f, 30.0f);

	// GL options

	GPU::setDepthState(true, TestFunction::LESS, true);
	GPU::setCullState(true, Faces::BACK);
	GPU::setBlendState(false, BlendEquation::ADD, BlendFunction::SRC_ALPHA, BlendFunction::ONE_MINUS_SRC_ALPHA);

	const int renderWidth  = int(resolution[0]);
	const int renderHeight = int(resolution[1]);
	_sceneFramebuffer	  = std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, {Layout::RGBA16F, Layout::R8, Layout::DEPTH_COMPONENT32F}, "G-buffer"));
	_lightingFramebuffer   = std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, Layout::RGBA8, "Lighting"));
	_preferredFormat.push_back(Layout::RGBA8);
	

	_fxaaProgram		= Resources::manager().getProgram2D("fxaa");
	_compositingProgram = Resources::manager().getProgram2D("game_composite");

	_ssaoPass = std::unique_ptr<SSAO>(new SSAO(renderWidth/2, renderHeight/2, 1, 1.5f, _name));
	_ssaoPass->quality() = SSAO::Quality::MEDIUM;
	
	_coloredProgram = Resources::manager().getProgram("colored_object");
	_ground			= Resources::manager().getMesh("ground", Storage::GPU);
	_head			= Resources::manager().getMesh("head", Storage::GPU);
	_bodyElement	= Resources::manager().getMesh("body", Storage::GPU);
	_cubemap		= Resources::manager().getTexture("env", Layout::RGBA8, Storage::GPU);

}

void GameRenderer::drawPlayer(const Player & player, Framebuffer & framebuffer) const {

	const glm::vec2 invRenderSize = 1.0f / glm::vec2(framebuffer.width(), framebuffer.height());

	// --- Scene pass ------
	_sceneFramebuffer->bind(glm::vec4(0.0f), 1.0f);
	_sceneFramebuffer->setViewport();

	drawScene(player);

	// --- SSAO pass ------
	_ssaoPass->process(_playerCamera.projection(), _sceneFramebuffer->depthBuffer(), _sceneFramebuffer->texture(0));

	GPU::setCullState(true, Faces::BACK);
	GPU::setBlendState(false);
	GPU::setDepthState(false);

	// --- Lighting pass ------
	_lightingFramebuffer->bind(Load::Operation::LOAD);
	_lightingFramebuffer->setViewport();
	_compositingProgram->use();
	_compositingProgram->textures({_sceneFramebuffer->texture(0), _sceneFramebuffer->texture(1), _ssaoPass->texture(), _cubemap});
	ScreenQuad::draw();

	// --- FXAA pass -------
	framebuffer.bind(Load::Operation::LOAD);
	framebuffer.setViewport();
	_fxaaProgram->use();
	_fxaaProgram->uniform("inverseScreenSize", invRenderSize);
	_fxaaProgram->texture(_lightingFramebuffer->texture(), 0);
	ScreenQuad::draw();

}

void GameRenderer::drawScene(const Player & player) const {
	GPU::setDepthState(true, TestFunction::LESS, true);
	GPU::setCullState(true, Faces::BACK);
	GPU::setBlendState(false);

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
		_coloredProgram->uniform("normalMat", glm::mat4(normalMatrix));
		_coloredProgram->uniform("matID", 1);
		GPU::drawMesh(*_ground);
	}
	// Render the head.
	{
		const glm::mat4 MVP			 = VP * player.modelHead;
		const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(player.modelHead)));
		_coloredProgram->uniform("mvp", MVP);
		_coloredProgram->uniform("normalMat", glm::mat4(normalMatrix));
		_coloredProgram->uniform("matID", 2);
		GPU::drawMesh(*_head);
	}
	// Render body elements and items.
	for(int i = 0; i < int(player.modelsBody.size()); ++i) {
		const glm::mat4 MVP			 = VP * player.modelsBody[i];
		const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(player.modelsBody[i])));
		_coloredProgram->uniform("mvp", MVP);
		_coloredProgram->uniform("normalMat", glm::mat4(normalMatrix));
		_coloredProgram->uniform("matID", player.looksBody[i]);
		GPU::drawMesh(*_bodyElement);
	}
	for(int i = 0; i < int(player.modelsItem.size()); ++i) {
		const glm::mat4 MVP			 = VP * player.modelsItem[i];
		const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(player.modelsItem[i])));
		_coloredProgram->uniform("mvp", MVP);
		_coloredProgram->uniform("normalMat", glm::mat4(normalMatrix));
		_coloredProgram->uniform("matID", player.looksItem[i]);
		GPU::drawMesh(*_bodyElement);
	}
}

void GameRenderer::resize(uint width, uint height) {
	const glm::vec2 res(width, height);
	_playerCamera.ratio(res[0]/res[1]);
	_sceneFramebuffer->resize(res);
	_lightingFramebuffer->resize(res);
	_ssaoPass->resize(width/2, height/2);
}
