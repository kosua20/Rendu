#pragma once

#include "Common.hpp"

/** \brief Represent a key-values tuple.
  	\ingroup Engine
 */
struct KeyValues {
	
	std::string key; ///< The key.
	std::vector<std::string> values; ///< A vector of values.
	std::vector<KeyValues> elements; ///< A vector of child elements.
	
	/** Constructor from a key.
	 \param aKey the key of the new tuple
	 */
	explicit KeyValues(const std::string & aKey);
};


/**
 \brief Contains configurable elements as attributes, populated from the command line, a configuration file or default values.
 \ingroup Engine
 */
class Config {
public:
	
	/** Initialize a new config object, parsing the input arguments and filling the attributes with their values.
	 	\param argv the raw input arguments
	 */
	explicit Config(const std::vector<std::string> & argv);
	
	/** Display help using the logger if the '--help' argument has been passed.
	 \return true if the help is shown, can be used for early exit
	 \note Arguments with no name but a description will be used as headers.
	 */
	bool showHelp();
	
protected:
	
	/** \brief Informations about an argument. */
	struct ArgumentInfo {
		
		/** Constructor.
		 \param aname argument long name
		 \param ashort argument optional short name
		 \param adetails argument description
		 \param avalues optional list of parameters for the argument
		 */
		ArgumentInfo(const std::string & aname,  const std::string & ashort, const std::string & adetails,
					 const std::vector<std::string> & avalues = {});
		
		/** Constructor.
		 \param aname argument name
		 \param ashort argument optional short name
		 \param adetails argument description
		 \param avalue parameter for the argument
		 */
		ArgumentInfo(const std::string & aname, const std::string & ashort, const std::string & adetails,
					 const std::string & avalue);
		
		std::string nameLong; ///< The main argument name.
		std::string nameShort; ///< The short argument name.
		std::string details; ///< Argument description.
		std::vector<std::string> values; ///< Zero, one or multiple argument parameters.
		
	};
	
	const std::vector<KeyValues> & arguments() const;
	
	std::vector<ArgumentInfo> & infos();
	
private:

	/**
	 Helper to extract (key, [values]) from a configuration file on disk.
	 \param filePath the path to the configuration file.
	 \param arguments a vector will be populated with (key, [values]) tuples.
	 */
	static void parseFromFile(const std::string & filePath, std::vector<KeyValues>& arguments);

	/**
	 Helper to extract (key, [values]) from the given command-line arguments.
	 \param argv the raw input arguments
	 \param arguments  a vector will be populated with (key, [values]) tuples.
	 */
	static void parseFromArgs(const std::vector<std::string> & argv, std::vector<KeyValues> & arguments);
	
	/// Store the internal parsed (keys, [values]) extracted from a file or the command-line.
	std::vector<KeyValues> _rawArguments;
	
	/// Store informations about each argument, for displaying the help message.
	std::vector<ArgumentInfo> _infos;
	
	bool _showHelp = false; ///< Should the help be displayed.
	
};

/**
 \brief Configuration containing parameters for windows and renderers.
 \ingroup Engine
 */
class RenderingConfig: public Config {
public:
	
	/** Initialize a new rendering config object, parsing the input arguments and filling the attributes with their values.
	 \param argv the raw input arguments
	 */
	explicit RenderingConfig(const std::vector<std::string> & argv);
	
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
	int internalVerticalResolution = 720;
	
	/// Should the aspect ratio of the window be constrained.
	bool forceAspectRatio = false;
	
	/// Size of the window in raw pixels, updated at launch based on screen density.
	glm::vec2 screenResolution = glm::vec2(800.0f,600.0f);
	
	/// The last recorded window position and size on screen.
	glm::ivec4 windowFrame = glm::vec4(0, 0, 800, 600);

};

