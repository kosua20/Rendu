#include "GameMenu.hpp"


MenuButton::MenuButton(const glm::vec2 & screenPos, const glm::vec2 & meshSize, const float screenScale, const int actionTag, const TextureInfos & texture){
	pos = screenPos;
	size = screenScale * meshSize;
	displayScale = screenScale;
	tag = actionTag;
	tid = texture.id;
}

MenuImage::MenuImage(const glm::vec2 & screenPos, const float screenScale, const TextureInfos & texture){
	pos = screenPos;
	size = screenScale * glm::vec2(1.0f, float(texture.height) / float(texture.width));
	tid = texture.id;
}

void GameMenu::update(const glm::vec2 & screenResolution, const float initialRatio){
	// Update the scaling of each button based on the screen ratio.
	const float currentRatio = screenResolution[0] / screenResolution[1];
	const float ratioFix = initialRatio / currentRatio;
	for( MenuButton & button : buttons){
		button.scale = button.displayScale * glm::vec2(ratioFix, 1.0f);
	}
	
	for( MenuImage & image : images){
		image.scale = image.size * glm::vec2(ratioFix, initialRatio);
	}
}
