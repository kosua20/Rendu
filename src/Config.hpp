//
//  Config.hpp
//  GL_Template
//
//  Created by Simon Rodriguez on 30/12/2017.
//  Copyright © 2017 Simon Rodriguez. All rights reserved.
//

#ifndef Config_hpp
#define Config_hpp
#include <glm/glm.hpp>
#include <stdio.h>

class Config {
public:
	
	Config(int argc, char** argv);

public:
	
	size_t version = 1;
	
	bool vsync = true;
	
	unsigned int initialWidth = 800;
	
	unsigned int initialHeight = 600;
	
	glm::vec2 screenResolution = glm::vec2(800.0,600.0);
	
	float internalVerticalResolution = 720.0f;
	
};
#endif /* Config_hpp */
