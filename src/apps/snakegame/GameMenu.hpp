#pragma once

#include "GameMenu.hpp"
#include "Common.hpp"


class GameMenu {
	
public:
	
	struct Button {
		glm::vec2 pos = glm::vec2(0.0f);
		glm::vec2 size = glm::vec2(100.0f);
		GLuint texture = 0;
		
		Button(const glm::vec2 & apos, const glm::vec2 & asize){
			pos = apos;
			size = asize;
		}
	};
	
	std::vector<Button> buttons;
	
	glm::vec3 backgroundColor = glm::vec3(0.0f);
};
