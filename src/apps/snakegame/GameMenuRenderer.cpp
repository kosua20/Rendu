
#include "GameMenuRenderer.hpp"
#include "Common.hpp"

GameMenuRenderer::GameMenuRenderer(RenderingConfig & config) : Renderer(config){
	
	_buttonProgram = Resources::manager().getProgram("menu_button");
	_button = Resources::manager().getMesh("plane");
}

void GameMenuRenderer::draw(const GameMenu & menu){
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, GLsizei(_config.screenResolution[0]), GLsizei(_config.screenResolution[1]));
	
	// Background color.
	glClearColor(menu.backgroundColor[0], menu.backgroundColor[1], menu.backgroundColor[2], 1.0f);
	glDisable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT);
	
	// Buttons
	glUseProgram(_buttonProgram->id());
	const float initialRatio = _config.initialWidth / float(_config.initialHeight);
	const float currentRatio = _config.screenResolution[0] / _config.screenResolution[1];
	const float ratioFix =  currentRatio / initialRatio;
	for(const auto & button : menu.buttons){
		const glm::vec2 finalScale =  _config.screenDensity * button.size * glm::vec2(1.0/ratioFix, 1.0f);
		glUniform2fv(_buttonProgram->uniform("position"), 1, &button.pos[0]);
		glUniform2fv(_buttonProgram->uniform("scale"), 1, &finalScale[0]);
		glUniform3f(_buttonProgram->uniform("color"), button.state == MenuButton::OFF, button.state == MenuButton::HOVER, button.state == MenuButton::ON);
		glBindVertexArray(_button.vId);
		glDrawElements(GL_TRIANGLES, _button.count, GL_UNSIGNED_INT, (void*)0);
	}
	glUseProgram(0);
	
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


