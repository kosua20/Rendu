#pragma once

#include "GameMenu.hpp"
#include "Common.hpp"

struct MenuButton {
	
public:
	
	MenuButton(const glm::vec2 & apos, const glm::vec2 & asize, const int atag);
	
	enum State {
		OFF, HOVER, ON
	};
	State state = OFF;
	
	glm::vec2 pos = glm::vec2(0.0f);
	glm::vec2 size = glm::vec2(100.0f);
	glm::vec2 scale = glm::vec2(1.0f);
	
	int tag = -1;
	
};


class GameMenu {
	
public:
	
	void update(const glm::vec2 & screenResolution, const float initialRatio);
	
	std::vector<MenuButton> buttons;
	
	glm::vec3 backgroundColor = glm::vec3(0.0f);
};
