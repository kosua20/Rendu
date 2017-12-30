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
#include <map>

class Config {
public:
	
	Config(int argc, char** argv);

public:
	
	const size_t version = 1;
	
	bool vsync = true;
	
	bool fullscreen = false;
	
	unsigned int initialWidth = 800;
	
	unsigned int initialHeight = 600;
	
	float internalVerticalResolution = 720.0f;
	
	glm::vec2 screenResolution = glm::vec2(800.0,600.0);

private:
	
	void parseFromFile(const char * filePath, std::map<std::string, std::string> & arguments);
	
	void parseFromArgs(const int argc, char** argv, std::map<std::string, std::string> & arguments);
};
#endif /* Config_hpp */
