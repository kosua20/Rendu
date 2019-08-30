#include "GameMenu.hpp"
#include "resources/Texture.hpp"

MenuButton::MenuButton(const glm::vec2 & screenPos, const glm::vec2 & meshSize, float screenScale, int actionTag, const Texture * texture) :
	pos(screenPos), size(screenScale * meshSize), scale(1.0f), displayScale(screenScale), tag(actionTag), tid(texture) {
}

bool MenuButton::contains(const glm::vec2 & mousePos) const {
	return glm::all(glm::greaterThanEqual(mousePos, pos - size * 0.5f))
		   && glm::all(glm::lessThanEqual(mousePos, pos + size * 0.5f));
}

MenuToggle::MenuToggle(const glm::vec2 & screenPos, const glm::vec2 & meshSize, float screenScale, int actionTag, const Texture * texture) :
	MenuButton(screenPos, meshSize, screenScale, actionTag, texture) {
	posBox   = screenPos + glm::vec2(2.0f / 3.0f, 0.0f) * screenScale;
	posImg   = screenPos - glm::vec2(0.4f, 0.0f) * screenScale;
	scaleBox = checkBoxScale * this->scale;
}

MenuImage::MenuImage(const glm::vec2 & screenPos, float screenScale, const Texture * texture) :
	pos(screenPos), scale(1.0f), tid(texture) {
	size = screenScale * glm::vec2(1.0f, float(texture->height) / float(texture->width));
}

MenuLabel::MenuLabel(const glm::vec2 & screenPos, float verticalScale, const Font * font, Font::Alignment alignment) :
	pos(screenPos), tid(font->atlas), _vScale(verticalScale), _font(font), _align(alignment) {
	update("0");
}

void MenuLabel::update(const std::string & text) {
	Font::generateLabel(text, *_font, _vScale, mesh, _align);
}

void GameMenu::update(const glm::vec2 & screenResolution, float initialRatio) {
	// Update the scaling of each button/toggle/image based on the screen ratio.
	const float currentRatio = screenResolution[0] / screenResolution[1];
	const float ratioFix	 = initialRatio / currentRatio;
	for(MenuButton & button : buttons) {
		button.scale = button.displayScale * glm::vec2(ratioFix, initialRatio);
	}
	for(MenuToggle & toggle : toggles) {
		toggle.scale	= toggle.displayScale * glm::vec2(ratioFix, initialRatio);
		toggle.scaleBox = toggle.checkBoxScale * toggle.scale;
	}
	for(MenuImage & image : images) {
		image.scale = image.size * glm::vec2(ratioFix, initialRatio);
	}
}
