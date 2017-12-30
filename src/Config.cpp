//
//  Config.cpp
//  GL_Template
//
//  Created by Simon Rodriguez on 30/12/2017.
//  Copyright © 2017 Simon Rodriguez. All rights reserved.
//

#include "Config.hpp"
#include "ResourcesManager.hpp"
#include <string>
#include <iostream>
#include <sstream>

Config::Config(int argc, char** argv){
	
	if(argc < 2){
		// Nothing to do, keep using default values.
		return;
	}
	
	std::map<std::string, std::string> arguments;
	
	// Have we received a config file as argument?
	const std::string potentialConfig = Resources::trim(std::string(argv[1]), "-");
	
	if(potentialConfig == "c" || potentialConfig == "config"){
		// Safety check.
		if(argc < 3){
			std::cerr << "[Config] Missing path for --config argument. Using default config." << std::endl;
			return;
		}
		parseFromFile(argv[2], arguments);
	} else {
		// Directly parse arguments.
		parseFromArgs(argc, argv, arguments);
	}
	
	for(const auto & arg : arguments){
		const std::string key = arg.first;
		const std::string value = arg.second;
		
		if(key == "novsync"){
			vsync = false;
		} else if(key == "internal-res" || key == "ivr"){
			internalVerticalResolution = std::stof(value);
		} else if(key == "wxh"){
			const std::string::size_type split = value.find_first_of("x");
			if(split != std::string::npos){
				unsigned int w = std::stoi(value.substr(0,split));
				unsigned int h = std::stoi(value.substr(split+1));
				initialWidth = w;
				initialHeight = h;
			}
		}
	}
	
}


void Config::parseFromFile(const char * filePath, std::map<std::string, std::string> & arguments){
	// Load config from given file.
	const std::string configContent = Resources::loadStringFromFile(filePath);
	if(configContent.empty()){
		std::cerr << "[Config] Missing/empty config file. Using default config." << std::endl;
		return;
	}
	std::istringstream lines(configContent);
	std::string line;
	while(std::getline(lines,line)){
		// Clean line.
		const std::string lineClean = Resources::trim(line, " ");
		// Split at first space.
		const std::string::size_type spacePos = lineClean.find_first_of(" ");
		if(spacePos == std::string::npos){
			// This is a on/off argument.
			arguments[Resources::trim(lineClean, "-")] = "true";
		} else {
			const std::string firstArg = Resources::trim(lineClean.substr(0, spacePos), "-");
			const std::string secondArg = Resources::trim(lineClean.substr(spacePos+1), " ");
			arguments[firstArg] = secondArg;
		}
	}
}


void Config::parseFromArgs(const int argc, char** argv, std::map<std::string, std::string> & arguments){
	for(uint argi = 1; argi < argc; ){
		// Clean the argument from any -
		const std::string firstArg = Resources::trim(std::string(argv[argi]), "-");
		if(argi < argc - 1){
			// We need to know if this is a on/off argument (the next argument is also a flag starting with a -)
			if(argv[argi+1][0] == '-'){
				// This is a on/off argument.
				arguments[firstArg] = "true";
				++argi;
			} else {
				const std::string secondArg = std::string(argv[argi+1]);
				arguments[firstArg] = secondArg;
				argi += 2;
			}
			
		} else {
			// If this was the last argument, there is no other after, so this is a on/off argument.
			arguments[firstArg] = "true";
			break;
		}
	}
}


