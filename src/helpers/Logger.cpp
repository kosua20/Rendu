#include "Logger.hpp"
#include <ctime>
#include <iomanip>
#include <iostream>
// We statically initialize the default logger.
// We don't really care about its exact construction/destruction moments,
// but we want it to always be created.
Log* Log::_defaultLogger = new Log();

void Log::set(LogLevel l){
	_level = l;
	_appendPrefix = (l != LogLevel::INFO);
}

Log::Log(){
	_level = LogLevel::INFO;
	_logToStdin = true;
	_verbose = false;
	_ignoreUntilFlush = false;
	_appendPrefix = false;
}

Log::Log(const std::string & filePath, const bool logToStdin, const bool verbose){
	_level = LogLevel::INFO;
	_logToStdin = logToStdin;
	_verbose = verbose;
	_ignoreUntilFlush = false;
	_appendPrefix = false;
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
		std::cerr << "[Logger] Unable to create log file at path " << filePath << "." << std::endl;
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
		
		const std::string finalStr =  _stream.str();
		
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
	_appendPrefix = false;
	_stream.str(std::string());
	_stream.clear();
	_level = LogLevel::INFO;
}

void Log::appendIfNeeded(){
	if(_appendPrefix){
		_appendPrefix = false;
		_stream << _levelStrings[_level];
	}
}

Log & Log::operator<<(const LogDomain& domain){
	if(domain != Verbose){
		_stream << "[" << _domainStrings[domain] << "] ";
		appendIfNeeded();
	} else if(!_verbose){
		// In this case, we want to ignore until the next flush.
		_ignoreUntilFlush = true;
	}
	return *this;
}

Log& Log::operator<<(std::ostream& (*modif)(std::ostream&)){
	appendIfNeeded();

	modif(_stream);
	if(modif == static_cast<std::ostream& (*)(std::ostream&)>(std::flush) ||
	   modif == static_cast<std::ostream& (*)(std::ostream&)>(std::endl)){
		flush();
	}
	return *this;
}

Log& Log::operator<<(std::ios_base& (*modif)(std::ios_base&)){
	modif(_stream);
	return *this;
}

// GLM types support.
Log & Log::operator<<(const glm::mat4& input){
	appendIfNeeded();
	_stream << "mat4( " << input[0][0] << ", " << input[0][1] << ", " << input[0][2] << ", " << input[0][3] << " | ";
	_stream << input[1][0] << ", " << input[1][1] << ", " << input[1][2] << ", " << input[1][3] << " | ";
	_stream <<  input[2][0] << ", " << input[2][1] << ", " << input[2][2] << ", " << input[2][3] << " | ";
	_stream << input[3][0] << ", " << input[3][1] << ", " << input[3][2] << ", " << input[3][3] << " )";
	return *this;
}

Log & Log::operator<<(const glm::mat3& input){
	appendIfNeeded();
	_stream << "mat3( " << input[0][0] << ", " << input[0][1] << ", " << input[0][2] << " | ";
	_stream << input[1][0] << ", " << input[1][1] << ", " << input[1][2] << " | ";
	_stream << input[2][0] << ", " << input[2][1] << ", " << input[2][2] << " )";
	return *this;
}

Log & Log::operator<<(const glm::mat2& input){
	appendIfNeeded();
	_stream << "mat2( " << input[0][0] << ", " << input[0][1] << " | ";
	_stream << input[1][0] << ", " << input[1][1] << " )";
	return *this;
}

Log & Log::operator<<(const glm::vec4& input){
	appendIfNeeded();
	_stream << "vec4( " << input[0] << ", " << input[1] << ", " << input[2] << ", " << input[3] << " )";
	return *this;
}

Log & Log::operator<<(const glm::vec3& input){
	appendIfNeeded();
	_stream << "vec3( " << input[0] << ", " << input[1] << ", " << input[2] << " )";
	return *this;
}

Log & Log::operator<<(const glm::vec2& input){
	appendIfNeeded();
	_stream << "vec2( " << input[0] << ", " << input[1] << " )";
	return *this;
}
