#include "system/Config.hpp"
#include "resources/ResourcesManager.hpp"
#include "system/TextUtilities.hpp"
#include <sstream>

KeyValues::KeyValues(const std::string & aKey) :
	key(aKey) {
}

Config::ArgumentInfo::ArgumentInfo(const std::string & aname, const std::string & ashort, const std::string & adetails,
	const std::vector<std::string> & avalues) :
	nameLong(aname),
	nameShort(ashort), details(adetails), values(avalues) {}

Config::Config(const std::vector<std::string> & argv) {

	if(argv.size() < 2) {
		// Nothing to do, keep using default values.
		return;
	}

	// Have we received a config file as argument?
	const std::string potentialConfig = TextUtilities::trim(argv[1], "-");

	if(potentialConfig == "c" || potentialConfig == "config") {
		// Safety check.
		if(argv.size() < 3) {
			Log::Error() << Log::Config << "Missing path for --config argument. Using default config." << std::endl;
			return;
		}
		parseFromFile(argv[2], _rawArguments);
	} else {
		// Directly parse arguments.
		parseFromArgs(argv, _rawArguments);
	}

	// Extract logging settings.
	std::string logPath;
	bool logVerbose = false;

	for(const auto & arg : arguments()) {
		if(arg.key == "verbose" || arg.key == "v") {
			logVerbose = true;
		} else if(arg.key == "log-path" && !arg.values.empty()) {
			logPath = arg.values[0];
		} else if(arg.key == "help" || arg.key == "h") {
			_showHelp = true;
		}
	}

	if(!logPath.empty()) {
		Log::setDefaultFile(logPath);
	}
	Log::setDefaultVerbose(logVerbose);

	registerSection("General");
	registerArgument("verbose", "v", "Enable the verbose log level.");
	registerArgument("log-path", "", "Log to a file instead of stdout.", "path/to/file.log");
	registerArgument("help", "h", "Show this help.");
	registerArgument("config", "c", "Load arguments from configuration file.", "path");
}

const std::vector<KeyValues> & Config::arguments() const {
	return _rawArguments;
}

void Config::registerSection(const std::string & name){
	_infos.emplace_back(std::string(), std::string(), name, std::vector<std::string>());
}

void Config::registerArgument(const std::string & longName, const std::string & shortName, const std::string & details, const std::vector<std::string> & params){
	_infos.emplace_back(longName, shortName, details, params);
}

void Config::registerArgument(const std::string & longName, const std::string & shortName, const std::string & details, const std::string & param){
	_infos.emplace_back(longName, shortName, details, std::vector<std::string>{param});
}

void Config::parseFromFile(const std::string & filePath, std::vector<KeyValues> & arguments) {
	// Load config from given file.
	const std::string configContent = Resources::loadStringFromExternalFile(filePath);
	if(configContent.empty()) {
		Log::Error() << Log::Config << "Missing/empty config file. Using default config." << std::endl;
		return;
	}
	std::istringstream lines(configContent);
	std::string line;
	while(std::getline(lines, line)) {
		// Clean line.
		const std::string lineClean = TextUtilities::trim(line, " ");
		if(lineClean.empty()) {
			continue;
		}
		// Split at first space.
		const std::string::size_type spacePos = lineClean.find_first_of(' ');
		std::vector<std::string> values;
		std::string firstArg;
		if(spacePos == std::string::npos) {
			// This is a on/off argument.
			firstArg = TextUtilities::trim(lineClean, "-");
		} else {
			// We need to split the whole line.
			firstArg = TextUtilities::trim(lineClean.substr(0, spacePos), "-");

			std::string::size_type beginPos	= spacePos + 1;
			std::string::size_type afterEndPos = lineClean.find_first_of(' ', beginPos);
			while(afterEndPos != std::string::npos) {
				const std::string value = lineClean.substr(beginPos, afterEndPos - beginPos);
				values.push_back(value);
				beginPos	= afterEndPos + 1;
				afterEndPos = lineClean.find_first_of(' ', beginPos);
			}
			// There is one remaining value, the last one.
			const std::string value = lineClean.substr(beginPos);
			values.push_back(value);
		}
		if(!firstArg.empty()) {
			arguments.emplace_back(firstArg);
			arguments.back().values = values;
		}
	}
}

void Config::parseFromArgs(const std::vector<std::string> & argv, std::vector<KeyValues> & arguments) {
	for(size_t argi = 1; argi < argv.size();) {
		// Clean the argument from any -
		const std::string firstArg = TextUtilities::trim(argv[argi], "-");
		if(firstArg.empty()) {
			continue;
		}
		std::vector<std::string> values;
		++argi;
		// While we do not encounter a dash, the values are associated to the current argument.
		while(argi < argv.size()) {
			const std::string currArg(argv[argi]);
			// If the argument begins with a double dash, it's the next argument.
			if(currArg.size() >= 2 && currArg.substr(0, 2) == "--") {
				break;
			}
			values.emplace_back(argv[argi]);
			++argi;
		}
		arguments.emplace_back(firstArg);
		arguments.back().values = values;
	}
}

bool Config::showHelp() {
	if(_showHelp) {
		// Estimate the longest character length, by building the names and values part of the descriptions.
		// Each line has the following format:
		// '  name1,name2 <value1> <value2> <value3>'
		std::vector<std::string> namesAndValues(_infos.size());
		size_t maxSize = 0;

		for(size_t aid = 0; aid < _infos.size(); ++aid) {
			auto & argInfos = _infos[aid];
			// If the name is empty, this is a header, insert its details as-is.
			if(argInfos.nameLong.empty()) {
				namesAndValues[aid] = " " + argInfos.details + ":";
				argInfos.details	= "";
				continue;
			}
			std::stringstream line;
			line << "  ";
			if(!argInfos.nameShort.empty()) {
				line << "--" << argInfos.nameShort << ",";
			}
			line << "--" << argInfos.nameLong;
			for(const auto & param : argInfos.values) {
				if(!param.empty()) {
					line << " <" << param << ">";
				}
			}
			namesAndValues[aid] = line.str();
			maxSize				= std::max(maxSize, namesAndValues[aid].size());
		}

		Log::Info() << Log::Config << "Help:" << std::endl;

		for(size_t aid = 0; aid < _infos.size(); ++aid) {
			const auto & argInfos = _infos[aid];
			Log::Info() << namesAndValues[aid];
			for(size_t i = namesAndValues[aid].size(); i < maxSize; ++i) {
				Log::Info() << " ";
			}
			Log::Info() << "  " << argInfos.details;
			Log::Info() << std::endl;
		}
	}
	return _showHelp;
}

RenderingConfig::RenderingConfig(const std::vector<std::string> & argv) :
	Config(argv) {
	for(const auto & arg : arguments()) {
		const std::string key					= arg.key;
		const std::vector<std::string> & values = arg.values;

		if(key == "no-vsync") {
			vsync = false;
		} else if(key == "half-rate") {
			rate = 30;
		} else if(key == "fullscreen") {
			fullscreen = true;
		} else if((key == "internal-res" || key == "ivr") && !values.empty()) {
			internalVerticalResolution = std::stoi(values[0]);
		} else if(key == "wxh" && values.size() >= 2) {
			const uint w  = uint(std::stoi(values[0]));
			const uint h  = uint(std::stoi(values[1]));
			initialWidth  = w;
			initialHeight = h;
		} else if(key == "force-aspect" || key == "far") {
			forceAspectRatio = true;
		}
	}

	registerSection("Rendering");
	registerArgument("no-vsync", "", "Disable V-sync.");
	registerArgument("half-rate", "", "30fps mode.");
	registerArgument("fullscreen", "", "Enable fullscreen.");
	registerArgument("internal-res", "ivr", "Vertical rendering resolution", "height.");
	registerArgument("wxh", "", "Window dimensions.", std::vector<std::string> {"width", "height"});
	registerArgument("force-aspect", "far", "Force window aspect ratio.");
}
