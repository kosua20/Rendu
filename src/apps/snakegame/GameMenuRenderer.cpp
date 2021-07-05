
#include "GameMenuRenderer.hpp"
#include "resources/ResourcesManager.hpp"
#include "graphics/ScreenQuad.hpp"
#include "graphics/GPU.hpp"
#include "graphics/Framebuffer.hpp"
#include "Common.hpp"

GameMenuRenderer::GameMenuRenderer() : Renderer("Menu") {
	_buttonProgram	   = Resources::manager().getProgram("menu_button");
	_backgroundProgram = Resources::manager().getProgram2D("passthrough");
	_imageProgram	   = Resources::manager().getProgram("menu_image");
	_fontProgram	   = Resources::manager().getProgram("font_sdf");
	_button			   = Resources::manager().getMesh("rounded-button-out", Storage::GPU);
	_buttonIn		   = Resources::manager().getMesh("rounded-button-in", Storage::GPU);
	_toggle			   = Resources::manager().getMesh("rounded-checkbox-out", Storage::GPU);
	_toggleIn		   = Resources::manager().getMesh("rounded-checkbox-in", Storage::GPU);
	_quad			   = Resources::manager().getMesh("plane", Storage::GPU);
}

void GameMenuRenderer::drawMenu(const GameMenu & menu, const glm::vec2 & finalRes, float aspectRatio) const {

	static const std::map<MenuButton::State, glm::vec4> borderColors = {
		{MenuButton::State::OFF, glm::vec4(0.8f, 0.8f, 0.8f, 1.0f)},
		{MenuButton::State::HOVER, glm::vec4(0.7f, 0.7f, 0.7f, 1.0f)},
		{MenuButton::State::ON, glm::vec4(0.95f, 0.95f, 0.95f, 1.0f)}};

	static const std::map<MenuButton::State, glm::vec4> innerColors = {
		{MenuButton::State::OFF, glm::vec4(0.9f, 0.9f, 0.9f, 0.5f)},
		{MenuButton::State::HOVER, glm::vec4(1.0f, 1.0f, 1.0f, 0.5f)},
		{MenuButton::State::ON, glm::vec4(0.95f, 0.95f, 0.95f, 0.5f)}};

	static const glm::vec4 labelsColor	 = glm::vec4(0.3f, 0.0f, 0.0f, 1.0f);
	static const glm::vec4 labelsEdgeColor = glm::vec4(1.0f);
	static const float labelsEdgeWidth	 = 0.25f;

	// Make sure we are rendering directly in the window.
	Framebuffer::backbuffer()->bind(Framebuffer::Load::LOAD, 1.0f);
	GPU::setViewport(0, 0, int(finalRes[0]), int(finalRes[1]));

	// Background image.
	if(menu.backgroundImage) {
		_backgroundProgram->use();
		ScreenQuad::draw(menu.backgroundImage);
	}

	GPU::setDepthState(true, TestFunction::LESS, true);
	GPU::setBlendState(true, BlendEquation::ADD, BlendFunction::SRC_ALPHA, BlendFunction::ONE_MINUS_SRC_ALPHA);

	// Images.
	_imageProgram->use();
	for(const auto & image : menu.images) {

		_imageProgram->uniform("position", image.pos);
		_imageProgram->uniform("scale", image.scale);

		_imageProgram->uniform("depth", 0.95f);
		GPU::bindTexture(image.tid, 0);
		GPU::drawMesh(*_quad);
	}

	// Buttons.
	for(const auto & button : menu.buttons) {
		_buttonProgram->use();
		_buttonProgram->uniform("position", button.pos);
		_buttonProgram->uniform("scale", button.scale);
		// Draw the inside half-transparent region.
		_buttonProgram->uniform("depth", 0.5f);
		_buttonProgram->uniform("color", innerColors.at(button.state));
		GPU::drawMesh(*_buttonIn);
		// Draw the border of the button.
		_buttonProgram->uniform("depth", 0.9f);
		_buttonProgram->uniform("color", borderColors.at(button.state));
		GPU::drawMesh(*_button);
		// Draw the text image.
		_imageProgram->use();
		_imageProgram->uniform("position", button.pos);
		const glm::vec2 newScale = button.scale * 0.7f * glm::vec2(1.0f, button.size[1] / button.size[0]);
		_imageProgram->uniform("scale", newScale);
		_imageProgram->uniform("depth", 0.2f);
		GPU::bindTexture(button.tid, 0);
		GPU::drawMesh(*_quad);
	}

	// Toggles.
	for(const auto & toggle : menu.toggles) {
		_buttonProgram->use();
		_buttonProgram->uniform("position", toggle.posBox);
		_buttonProgram->uniform("scale", toggle.scaleBox);
		_buttonProgram->uniform("depth", 0.9f);
		// Outside border.
		_buttonProgram->uniform("color", borderColors.at(MenuButton::State::OFF));
		GPU::drawMesh(*_toggle);
		// If checked, fill the box.
		if(toggle.state == MenuButton::State::ON) {
			_buttonProgram->uniform("color", innerColors.at(MenuButton::State::OFF));
			GPU::drawMesh(*_toggleIn);
		}
		// Text display.
		const glm::vec2 newScale = toggle.scale * 0.7f * glm::vec2(1.0f, toggle.size[1] / toggle.size[0]);
		_imageProgram->use();
		_imageProgram->uniform("position", toggle.posImg);
		_imageProgram->uniform("scale", newScale);
		_imageProgram->uniform("depth", 0.2f);
		GPU::bindTexture(toggle.tid, 0);
		GPU::drawMesh(*_quad);
	}
	GPU::setDepthState(false);

	// Labels
	_fontProgram->use();
	for(const auto & label : menu.labels) {
		GPU::bindTexture(label.tid, 0);
		_fontProgram->uniform("ratio", aspectRatio);
		_fontProgram->uniform("position", label.pos);
		_fontProgram->uniform("color", labelsColor);
		_fontProgram->uniform("edgeColor", labelsEdgeColor);
		_fontProgram->uniform("edgeWidth", labelsEdgeWidth);
		GPU::drawMesh(label.mesh);
	}
	GPU::setBlendState(false);
	checkGPUError();
}

glm::vec2 GameMenuRenderer::getButtonSize() const {
	return glm::vec2(_button->bbox.getSize());
}
