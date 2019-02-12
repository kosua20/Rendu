
#include "GameRenderer.hpp"
#include "Common.hpp"
#include "helpers/InterfaceUtilities.hpp"
#include "input/Input.hpp"

GameRenderer::GameRenderer(RenderingConfig & config) : Renderer(config){
	
	_playerCamera.pose(glm::vec3(0.0f, 0.0f, 25.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	_playerCamera.projection(config.screenResolution[0]/config.screenResolution[1], 0.6f, 1.0f, 30.0f);
	_player.resize(_renderResolution);
	// GL options
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	const int renderWidth = (int)_renderResolution[0];
	const int renderHeight = (int)_renderResolution[1];
	_sceneFramebuffer = std::make_shared<Framebuffer>(renderWidth, renderHeight, GL_RGBA8, true);
	_fxaaFramebuffer = std::make_shared<Framebuffer>(renderWidth, renderHeight, GL_RGBA8, false);
	_fxaaProgram = Resources::manager().getProgram2D("fxaa");
	_finalProgram = Resources::manager().getProgram2D("final_screenquad");
	
	_startTime = 0.0;
}

void GameRenderer::draw(){
	const glm::vec2 invRenderSize = 1.0f/_renderResolution;
	
	
	_sceneFramebuffer->bind();
	_sceneFramebuffer->setViewport();
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// Render the ground.
	const auto groundProgram = Resources::manager().getProgram("colored_object");
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
	_player.draw(_playerCamera.view(), _playerCamera.projection());
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
	glEnable(GL_DEPTH_TEST);
	
	checkGLError();
	
	
}

void GameRenderer::update(){
	Renderer::update();
	if(ImGui::Begin("Infos")){
		ImGui::Text("%.1f ms, %.1f fps", ImGui::GetIO().DeltaTime*1000.0f, ImGui::GetIO().Framerate);
	}
	ImGui::End();
	
	_player.update();
}

void GameRenderer::physics(double fullTime, double frameTime){
	if(_startTime == 0.0){
		_startTime = fullTime;
	}
	
	_player.physics(fullTime - _startTime, frameTime);
	
	
}

void GameRenderer::resize(unsigned int width, unsigned int height){
	Renderer::updateResolution(width, height);
	const float aspectRatio = _renderResolution[0]/_renderResolution[1];
	_playerCamera.ratio(aspectRatio);
	_fxaaFramebuffer->resize(_renderResolution);
	_sceneFramebuffer->resize(_renderResolution);
	_player.resize(_renderResolution);
}

void GameRenderer::clean() const {
	Renderer::clean();
	_fxaaFramebuffer->clean();
	_sceneFramebuffer->clean();
	_player.clean();
}
