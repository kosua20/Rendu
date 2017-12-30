//
//  Config.cpp
//  GL_Template
//
//  Created by Simon Rodriguez on 30/12/2017.
//  Copyright © 2017 Simon Rodriguez. All rights reserved.
//

#include "Config.hpp"
#include <string>

Config::Config(int argc, char** argv){
	
	if(argc < 2){
		// Nothing to do, keep using default values.
		return;
	}
	
	const std::string firstArg = std::string(argv[1]);
	if(firstArg == "-config"){
		// Load config from given file.
		// TODO: load config from file.
	} else {
		// TODO: load config from arguments.
	}
}
