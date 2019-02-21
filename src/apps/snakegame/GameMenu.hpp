#pragma once

#include "GameMenu.hpp"
#include "Common.hpp"

struct MenuButton {
	
	
	glm::vec2 pos = glm::vec2(0.0f);
	glm::vec2 size = glm::vec2(100.0f);
	GLuint texture = 0;
	int tag = -1;
	
	enum State {
		OFF, HOVER, ON
	};
	
	State state = OFF;
	
	MenuButton(const glm::vec2 & apos, const glm::vec2 & asize, const int atag){
		pos = apos;
		size = asize;
		tag = atag;
	}
	
	
};


class GameMenu {
	
public:
	
	std::vector<MenuButton> buttons;
	
	glm::vec3 backgroundColor = glm::vec3(0.0f);
};
