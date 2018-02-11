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
#include <string>

class Config {
public:
	
	Config(int argc, char** argv);

public:
	
	/// Configuration properties (loaded from command-line or config file).
	const size_t version = 1;
	
	bool vsync = true;
	
	bool fullscreen = false;
	
	bool logVerbose = false;
	
	unsigned int initialWidth = 800;
	
	unsigned int initialHeight = 600;
	
	float internalVerticalResolution = 720.0f;
	
	/// Computed properties.
	glm::vec2 screenResolution = glm::vec2(800.0,600.0);
	
	float screenDensity = 1.0f;
	
	std::string logPath = "";
	
public:
	
	static void parseFromFile(const char * filePath, std::map<std::string, std::string> & arguments);
	
	static void parseFromArgs(const int argc, char** argv, std::map<std::string, std::string> & arguments);
		
	void processArguments();

protected:
	
	std::map<std::string, std::string> _rawArguments;
};
#endif /* Config_hpp */
