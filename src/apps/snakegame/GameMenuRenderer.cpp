
#include "GameMenuRenderer.hpp"
#include "resources/ResourcesManager.hpp"
#include "graphics/ScreenQuad.hpp"
#include "input/Input.hpp"
#include "Common.hpp"

GameMenuRenderer::GameMenuRenderer(RenderingConfig & config) : Renderer(config){
	
	_buttonProgram = Resources::manager().getProgram("menu_button");
	_backgroundProgram = Resources::manager().getProgram2D("passthrough");
	_imageProgram = Resources::manager().getProgram("menu_image");
	_button = Resources::manager().getMesh("rounded-button-out");
	_buttonIn = Resources::manager().getMesh("rounded-button-in");
	_toggle = Resources::manager().getMesh("rounded-checkbox-out");
	_toggleIn = Resources::manager().getMesh("rounded-checkbox-in");
	_quad = Resources::manager().getMesh("plane");
}

void GameMenuRenderer::draw(const GameMenu & menu){
	
	static const std::map<MenuButton::State, glm::vec4> borderColors = {
		{ MenuButton::OFF, glm::vec4(0.8f, 0.8f, 0.8f, 1.0f) },
		{ MenuButton::HOVER, glm::vec4(0.7f, 0.7f, 0.7f, 1.0f) },
		{ MenuButton::ON, glm::vec4(0.95f, 0.95f , 0.95f, 1.0f) }
	};
	
	static const std::map<MenuButton::State, glm::vec4> innerColors = {
		{ MenuButton::OFF, glm::vec4(0.9f, 0.9f, 0.9f, 0.5f) },
		{ MenuButton::HOVER, glm::vec4(1.0f, 1.0f, 1.0f, 0.5f) },
		{ MenuButton::ON, glm::vec4(0.95f, 0.95f , 0.95f, 0.5f) }
	};
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_FRAMEBUFFER_SRGB);
	glViewport(0, 0, GLsizei(_config.screenResolution[0]), GLsizei(_config.screenResolution[1]));
	glClear(GL_DEPTH_BUFFER_BIT);
	
	// Background image.
	if(menu.backgroundImage > 0){
		glUseProgram(_backgroundProgram->id());
		ScreenQuad::draw(menu.backgroundImage);
	}
	
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	
	// Images.
	glUseProgram(_imageProgram->id());
	for(const auto & image : menu.images){
		
		glUniform2fv(_imageProgram->uniform("position"), 1, &image.pos[0]);
		glUniform2fv(_imageProgram->uniform("scale"), 1, &image.scale[0]);
		
		glUniform1f(_imageProgram->uniform("depth"), 0.95f);
		glActiveTexture(GL_TEXTURE0 );
		glBindTexture(GL_TEXTURE_2D, image.tid);
		
		glBindVertexArray(_quad.vId);
		glDrawElements(GL_TRIANGLES, _quad.count, GL_UNSIGNED_INT, (void*)0);
	}
	
	// Buttons.
	for(const auto & button : menu.buttons){
		glUseProgram(_buttonProgram->id());
		glUniform2fv(_buttonProgram->uniform("position"), 1, &button.pos[0]);
		glUniform2fv(_buttonProgram->uniform("scale"), 1, &button.scale[0]);
		// Draw the inside half-transparent region.
		glUniform1f(_buttonProgram->uniform("depth"), 0.5f);
		glUniform4fv(_buttonProgram->uniform("color"), 1, &innerColors.at(button.state)[0]);
		glBindVertexArray(_buttonIn.vId);
		glDrawElements(GL_TRIANGLES, _buttonIn.count, GL_UNSIGNED_INT, (void*)0);
		// Draw the border of the button.
		glUniform1f(_buttonProgram->uniform("depth"), 0.9f);
		glUniform4fv(_buttonProgram->uniform("color"), 1, &borderColors.at(button.state)[0]);
		glBindVertexArray(_button.vId);
		glDrawElements(GL_TRIANGLES, _button.count, GL_UNSIGNED_INT, (void*)0);
		// Draw the text image.
		glUseProgram(_imageProgram->id());
		glUniform2fv(_imageProgram->uniform("position"), 1, &button.pos[0]);
		const glm::vec2 newScale = button.scale * 0.7f * glm::vec2(1.0f, button.size[1]/button.size[0]);
		glUniform2fv(_imageProgram->uniform("scale"), 1, &newScale[0]);
		glUniform1f(_imageProgram->uniform("depth"), 0.2f);
		glActiveTexture(GL_TEXTURE0 );
		glBindTexture(GL_TEXTURE_2D, button.tid);
		glBindVertexArray(_quad.vId);
		glDrawElements(GL_TRIANGLES, _quad.count, GL_UNSIGNED_INT, (void*)0);
		
	}
	
	// Toggles.
	for(const auto & toggle : menu.toggles){
		glUseProgram(_buttonProgram->id());
		glUniform2fv(_buttonProgram->uniform("position"), 1, &toggle.posBox[0]);
		glUniform2fv(_buttonProgram->uniform("scale"), 1, &toggle.scaleBox[0]);
		glUniform1f(_buttonProgram->uniform("depth"), 0.9f);
		// Outside border.
		glUniform4fv(_buttonProgram->uniform("color"), 1, &borderColors.at(MenuButton::OFF)[0]);
		glBindVertexArray(_toggle.vId);
		glDrawElements(GL_TRIANGLES, _toggle.count, GL_UNSIGNED_INT, (void*)0);
		// If checked, fill the box.
		if(toggle.state == MenuButton::ON){
			glUniform4fv(_buttonProgram->uniform("color"), 1, &innerColors.at(MenuButton::OFF)[0]);
			glBindVertexArray(_toggleIn.vId);
			glDrawElements(GL_TRIANGLES, _toggleIn.count, GL_UNSIGNED_INT, (void*)0);
		}
		// Text display.
		const glm::vec2 newScale = toggle.scale * 0.7f * glm::vec2(1.0f, toggle.size[1]/toggle.size[0]);
		glUseProgram(_imageProgram->id());
		glUniform2fv(_imageProgram->uniform("position"), 1, &toggle.posImg[0]);
		glUniform2fv(_imageProgram->uniform("scale"), 1, &newScale[0]);
		glUniform1f(_imageProgram->uniform("depth"), 0.2f);
		glActiveTexture(GL_TEXTURE0 );
		glBindTexture(GL_TEXTURE_2D, toggle.tid);
		glBindVertexArray(_quad.vId);
		glDrawElements(GL_TRIANGLES, _quad.count, GL_UNSIGNED_INT, (void*)0);
		
	}
	glUseProgram(0);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_FRAMEBUFFER_SRGB);
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

glm::vec2 GameMenuRenderer::getButtonSize(){
	return glm::vec2(_button.bbox.getSize());
}


