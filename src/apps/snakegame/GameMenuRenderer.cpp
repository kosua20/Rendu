
#include "GameMenuRenderer.hpp"
#include "Common.hpp"

GameMenuRenderer::GameMenuRenderer(RenderingConfig & config) : Renderer(config){
	
}

void GameMenuRenderer::draw(const GameMenu & menu){
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, GLsizei(_config.screenResolution[0]), GLsizei(_config.screenResolution[1]));
	glClearColor(menu.backgroundColor[0], menu.backgroundColor[1], menu.backgroundColor[2], 1.0f);
	glDisable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT);
	checkGLError();
}

void GameMenuRenderer::update(){
	Renderer::update();
}

void GameMenuRenderer::resize(unsigned int width, unsigned int height){
	Renderer::updateResolution(width, height);
}

void GameMenuRenderer::clean() const {
	Renderer::clean();
}


