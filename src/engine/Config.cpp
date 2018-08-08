//
//  Config.cpp
//  GL_Template
//
//  Created by Simon Rodriguez on 30/12/2017.
//  Copyright © 2017 Simon Rodriguez. All rights reserved.
//

#include "Config.hpp"
#include "resources/ResourcesManager.hpp"

#include <sstream>

Config::Config(int argc, char** argv){
	
	if(argc < 2){
		// Nothing to do, keep using default values.
		return;
	}
	
	// Have we received a config file as argument?
	const std::string potentialConfig = Resources::trim(std::string(argv[1]), "-");
	
	if(potentialConfig == "c" || potentialConfig == "config"){
		// Safety check.
		if(argc < 3){
			Log::Error() << Log::Config << "Missing path for --config argument. Using default config." << std::endl;
			return;
		}
		parseFromFile(argv[2], _rawArguments);
	} else {
		// Directly parse arguments.
		parseFromArgs(argc, argv, _rawArguments);
	}
	
	processArguments();
	
}

void Config::processArguments(){
	
	for(const auto & arg : _rawArguments){
		const std::string key = arg.first;
		const std::vector<std::string> & values = arg.second;
		
		if(key == "novsync"){
			vsync = false;
		} else if(key == "fullscreen"){
			fullscreen = true;
		} else if(key == "verbose"){
			logVerbose = true;
		} else if(key == "internal-res" || key == "ivr"){
			internalVerticalResolution = std::stof(values[0]);
		} else if(key == "log-path"){
			logPath = values[0];
		} else if(key == "wxh"){
			const unsigned int w = (unsigned int)std::stoi(values[0]);
			const unsigned int h = (unsigned int)std::stoi(values[1]);
			initialWidth = w;
			initialHeight = h;
		}
	}
}


void Config::parseFromFile(const char * filePath, std::map<std::string, std::vector<std::string>> & arguments){
	// Load config from given file.
	const std::string configContent = Resources::loadStringFromExternalFile(filePath);
	if(configContent.empty()){
		Log::Error() << Log::Config << "Missing/empty config file. Using default config." << std::endl;
		return;
	}
	std::istringstream lines(configContent);
	std::string line;
	while(std::getline(lines,line)){
		// Clean line.
		const std::string lineClean = Resources::trim(line, " ");
		if(lineClean.empty()) {
			continue;
		}
		// Split at first space.
		const std::string::size_type spacePos = lineClean.find_first_of(" ");
		std::vector<std::string> values;
		std::string firstArg = "";
		if(spacePos == std::string::npos){
			// This is a on/off argument.
			firstArg = Resources::trim(lineClean, "-");
		} else {
			// We need to split the whole line.
			firstArg = Resources::trim(lineClean.substr(0, spacePos), "-");
			
			std::string::size_type beginPos = spacePos+1;
			std::string::size_type afterEndPos = lineClean.find_first_of(" ", beginPos);
			while (afterEndPos != std::string::npos) {
				const std::string value = lineClean.substr(beginPos, afterEndPos - beginPos);
				values.push_back(value);
				beginPos = afterEndPos + 1;
				afterEndPos = lineClean.find_first_of(" ", beginPos);
			}
			// There is one remaining value, the last one.
			const std::string value = lineClean.substr(beginPos);
			values.push_back(value);
			
		}
		if(!firstArg.empty()) {
			arguments[firstArg] = values;
		}
	}
}


void Config::parseFromArgs(const int argc, char** argv, std::map<std::string, std::vector<std::string>> & arguments){
	for(size_t argi = 1; argi < (size_t)argc; ){
		// Clean the argument from any -
		const std::string firstArg = Resources::trim(std::string(argv[argi]), "-");
		if (firstArg.empty()) {
			continue;
		}
		std::vector<std::string> values;
		++argi;
		// While we do not encounter a dash, the values are associated to the current argument.
		while (argi < (size_t)argc && argv[argi][0] != '-') {
			values.emplace_back(argv[argi]);
			++argi;
		}
		arguments[firstArg] = values;

	}
}



