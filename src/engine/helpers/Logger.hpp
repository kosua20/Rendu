#ifndef Logger_h
#define Logger_h

#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <glm/glm.hpp>
// Fix for Windows headers.
#ifdef ERROR
#undef ERROR
#endif
class Log {
	
public:
	
	enum LogDomain {
		OpenGL, Resources, Input, Utilities, Config, Verbose
	};

private:
	
	enum LogLevel {
		INFO, WARNING, ERROR
	};
	
	void set(LogLevel l);

	const std::vector<std::string> _domainStrings = {"OpenGL","Resources","Input","Utilities","Config",""};

	const std::vector<std::string> _levelStrings = {"","(!) ","(X) "};
	
public:
	
	Log();
	
	Log(const std::string & filePath, const bool logToStdin, const bool verbose = false);
	
	void setFile(const std::string & filePath, const bool flushExisting = true);
	
	void setVerbose(const bool verbose);
	
	template<class T>
	Log & operator<<(const T& input){
		appendIfNeeded();
		_stream << input;
		return *this;
	}
	
	Log & operator<<(const LogDomain& domain);
	
	Log& operator<<(std::ostream& (*modif)(std::ostream&));
	
	Log& operator<<(std::ios_base& (*modif)(std::ios_base&));
	
	// GLM types support.
	Log & operator<<(const glm::mat4& input);
	
	Log & operator<<(const glm::mat3& input);
	
	Log & operator<<(const glm::mat2& input);
	
	Log & operator<<(const glm::vec4& input);
	
	Log & operator<<(const glm::vec3& input);
	
	Log & operator<<(const glm::vec2& input);
	
public:
	
	static void setDefaultFile(const std::string & filePath);
	
	static void setDefaultVerbose(const bool verbose);
	
	static Log& Info();
	
	static Log& Warning();
	
	static Log& Error();

private:
	
	void flush();

	void appendIfNeeded();
	
	LogLevel _level;
	bool _logToStdin;
	std::ofstream _file;
	std::stringstream _stream;
	bool _verbose;
	bool _ignoreUntilFlush;
	bool _appendPrefix;
	
	static Log* _defaultLogger;
};

#endif
