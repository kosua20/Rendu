//
//  Config.hpp
//  GL_Template
//
//  Created by Simon Rodriguez on 30/12/2017.
//  Copyright © 2017 Simon Rodriguez. All rights reserved.
//

#ifndef Config_hpp
#define Config_hpp
#include "Common.hpp"
#include <map>

/**
 \brief Contains configurable elements as attributes, populated from the command line, a configuration file or default values.
 \ingroup Engine
 */
class Config {
public:
	
	/** Initialize a new config object, parsing the input arguments and filling the attributes with their values.
	 	\param argc the number of input arguments.
	 	\param argv a pointer to the raw input arguments.
	 */
	Config(int argc, char** argv);
	
private:
	
	/**
	 Helper to extract (key, [values]) from a configuration file on disk.
	 \param filePath the path to the configuration file.
	 \param arguments a dictionary that will be populated with (key, [values]).
	 */
	static void parseFromFile(const char* filePath, std::map<std::string, std::vector<std::string>>& arguments);
	
	/**
	 Helper to extract (key, [values]) from the given command-line raw C-style arguments.
	 \param argc the number of input arguments.
	 \param argv a pointer to the raw input arguments.
	 \param arguments a dictionary that will be populated with (key, [values]).
	 */
	static void parseFromArgs(const int argc, char** argv, std::map<std::string, std::vector<std::string>> & arguments);

protected:
	
	/// Store the internal parsed (keys, [values]) extracted from a file or the command-line.
	std::map<std::string, std::vector<std::string>> _rawArguments;
};


/**
 \brief Configuration containing parameters for windows and renderers.
 \ingroup Engine
 */
class RenderingConfig: public Config {
public:
	
	/** Initialize a new rendering config object, parsing the input arguments and filling the attributes with their values.
	 \param argc the number of input arguments.
	 \param argv a pointer to the raw input arguments.
	 */
	RenderingConfig(int argc, char** argv);

public:
	// Configuration properties (loaded from command-line or config file).
	
	/// The configuration version number (unused).
	const size_t version = 1;
	
	/// Toggle V-Sync.
	bool vsync = true;
	
	/// Prefered framerate.
	int rate = 60;
	
	/// Toggle fullscreen window.
	bool fullscreen = false;
	
	/// Initial width of the window in relative pixels.
	unsigned int initialWidth = 800;
	
	/// Initial height of the window in relative pixels.
	unsigned int initialHeight = 600;
	
	/// \brief Internal vertical rendering resolution.
	/// \note The width should be computed based on the window aspect ratio.
	float internalVerticalResolution = 720.0f;
	
	/// Size of the window in raw pixels, updated at launch based on screen density.
	glm::vec2 screenResolution = glm::vec2(800.0,600.0);
	
	/// Screen density, udpated at launch.
	float screenDensity = 1.0f;
	
protected:
	
	/**
	 Read the internal (key, [values]) populated dictionary, and transfer their values to the configuration attributes.
	 */
	void processArguments();
	
};

#endif /* Config_hpp */
