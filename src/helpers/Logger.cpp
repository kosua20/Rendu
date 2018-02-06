#include "Logger.hpp"

// We statically initialize the default logger.
// We don't really care about its exact construction/destruction moments,
// but we want it to always be created.
Log* Log::_defaultLogger = new Log();

void Log::set(LogLevel l){
	_level = l;
}

Log::Log(){
	_level = LogLevel::INFO;
	_logToStdin = true;
	_verbose = false;
	_ignoreUntilFlush = false;
}

Log::Log(const std::string & filePath, const bool logToStdin, const bool verbose){
	_level = LogLevel::INFO;
	_logToStdin = logToStdin;
	_verbose = verbose;
	_ignoreUntilFlush = false;
	// Create file if it doesnt exist.
	setFile(filePath, false);
}

void Log::setFile(const std::string & filePath, const bool flushExisting){
	if(flushExisting){
		_stream << std::endl;
		flush();
	}
	if(_file.is_open()){
		_file.close();
	}
	_file.open(filePath, std::ofstream::app);
	if(_file.is_open()){
		_file << "-- New session - " << time(NULL) << " -------------------------------" << std::endl;
	} else {
		std::cerr() << "[Logger] Unable to create log file at path " << filePath << "." << std::endl;
	}
}

void Log::setVerbose(const bool verbose){
	_verbose = verbose;
}




void Log::setDefaultFile(const std::string & filePath){
	_defaultLogger->setFile(filePath);
}

void Log::setDefaultVerbose(const bool verbose){
	_defaultLogger->setVerbose(verbose);
}

Log& Log::Info(){
	_defaultLogger->set(LogLevel::INFO);
	return *_defaultLogger;
}

Log& Log::Warning(){
	_defaultLogger->set(LogLevel::WARNING);
	return *_defaultLogger;
}

Log& Log::Error(){
	_defaultLogger->set(LogLevel::ERROR);
	return *_defaultLogger;
}

void Log::flush(){
	if(!_ignoreUntilFlush){
		
		const std::string levelPrefix = (_level == LogLevel::ERROR ? "(X) " :  (_level == LogLevel::WARNING ? "(!) " : ""));
		const std::string finalStr = levelPrefix + _stream.str();
		
		if(_logToStdin){
			if(_level == LogLevel::INFO){
				std::cout << finalStr << std::flush;
			} else {
				std::cerr << finalStr << std::flush;
			}
		}
		if(_file.is_open()){
			_file << finalStr << std::flush;
		}
	}
	_ignoreUntilFlush = false;
	_stream.str(std::string());
	_stream.clear();
	_level = LogLevel::INFO;
}
