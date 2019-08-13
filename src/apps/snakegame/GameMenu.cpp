#include "GameMenu.hpp"
#include "graphics/GPUObjects.hpp"
#include "resources/Texture.hpp"

MenuButton::MenuButton(const glm::vec2 & screenPos, const glm::vec2 & meshSize, const float screenScale, const int actionTag, const Texture & texture){
	pos = screenPos;
	size = screenScale * meshSize;
	displayScale = screenScale;
	tag = actionTag;
	tid = texture.gpu->id;
}

bool MenuButton::contains(const glm::vec2 & mousePos){
	return glm::all(glm::greaterThanEqual(mousePos, pos - size * 0.5f))
	&& glm::all(glm::lessThanEqual(mousePos, pos + size * 0.5f));
}

MenuToggle::MenuToggle(const glm::vec2 & screenPos, const glm::vec2 & meshSize, const float screenScale, const int actionTag, const Texture & texture) : MenuButton(screenPos, meshSize, screenScale, actionTag, texture){
	posBox = this->pos + glm::vec2(2.0f/3.0f, 0.0f)*screenScale;
	posImg = this->pos - glm::vec2(0.4f, 0.0f)*screenScale;
	scaleBox = checkBoxScale * this->scale;
}

MenuImage::MenuImage(const glm::vec2 & screenPos, const float screenScale, const Texture & texture){
	pos = screenPos;
	size = screenScale * glm::vec2(1.0f, float(texture.height) / float(texture.width));
	tid = texture.gpu->id;
}

MenuLabel::MenuLabel(const glm::vec2 & screenPos, const float verticalScale, const FontInfos * font, const Font::Alignment alignment) {
	_font = font;
	pos = screenPos;
	tid = _font->atlas->gpu->id;
	_vScale = verticalScale;
	_align = alignment;
	update("0");
}

void MenuLabel::update(const std::string & text){
	mesh.clean();
	mesh = Font::generateLabel(text, *_font, _vScale, _align);
}

void GameMenu::update(const glm::vec2 & screenResolution, const float initialRatio){
	// Update the scaling of each button/toggle/image based on the screen ratio.
	const float currentRatio = screenResolution[0] / screenResolution[1];
	const float ratioFix = initialRatio / currentRatio;
	for( MenuButton & button : buttons){
		button.scale = button.displayScale * glm::vec2(ratioFix, initialRatio);
	}
	for( MenuToggle & toggle : toggles){
		toggle.scale = toggle.displayScale * glm::vec2(ratioFix, initialRatio);
		toggle.scaleBox = toggle.checkBoxScale * toggle.scale;
	}
	for( MenuImage & image : images){
		image.scale = image.size * glm::vec2(ratioFix, initialRatio);
	}
}
