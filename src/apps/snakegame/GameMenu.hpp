#pragma once

#include "Common.hpp"
#include "GameMenu.hpp"
#include "graphics/GLUtilities.hpp"


struct MenuButton {
	
public:
	
	MenuButton(const glm::vec2 & screenPos, const glm::vec2 & meshSize, const float screenScale, const int actionTag);
	
	enum State {
		OFF, HOVER, ON
	};
	State state = OFF;
	
	glm::vec2 pos;
	glm::vec2 size;
	glm::vec2 scale;
	float displayScale;
	
	int tag;
};

struct MenuImage {
	
public:
	
	MenuImage(const glm::vec2 & screenPos, const float screenScale, const TextureInfos & texture);
	
	glm::vec2 pos;
	glm::vec2 size;
	glm::vec2 scale;
	
	GLuint tid;
	
};


class GameMenu {
	
public:
	
	void update(const glm::vec2 & screenResolution, const float initialRatio);
	
	std::vector<MenuButton> buttons;
	std::vector<MenuImage> images;
	
	GLuint backgroundImage = 0;
};
