#include "GameMenu.hpp"


MenuButton::MenuButton(const glm::vec2 & apos, const glm::vec2 & asize, const int atag){
	pos = apos;
	size = asize;
	tag = atag;
}


void GameMenu::update(const glm::vec2 & screenResolution, const float initialRatio){
	// Update the scaling of each button based on the screen ratio.
	const float currentRatio = screenResolution[0] / screenResolution[1];
	const float ratioFix =   initialRatio / currentRatio;
	for( MenuButton & button : buttons){
		button.scale = button.size * glm::vec2(ratioFix, 1.0f);
	}
}
