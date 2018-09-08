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

/**
 \brief Provides logging utilities, either to the standard/error output or to a file, with multiple criticality levels.
 \ingroup Helpers
 */
class Log {
	
public:
	
	/// \brief Domain prefix that will be appended before a line.
	enum LogDomain {
		OpenGL, Resources, Input, Utilities, Config,
		Verbose ///< Will only be logged if verbose is enabled.
	};

private:
	
	/// \brief Criticality level
	enum LogLevel {
		INFO, WARNING, ERROR
	};
	
	/** Set the criticality level.
	 \param l the new level
	 \note Level is not modified directly, but through the use of Log::Info, Log::Error, etc., mimicking std::cout and std::cerr.
	 */
	void set(LogLevel l);

	const std::vector<std::string> _domainStrings = {"OpenGL","Resources","Input","Utilities","Config",""}; ///< Domain prefix strings.

	const std::vector<std::string> _levelStrings = {"","(!) ","(X) "}; ///< Levels prefix strings.
	
public:
	
	/** Default constructor, will use standard output */
	Log();
	
	/** Constructor
	 \param filePath the file to write the logs to
	 \param logToStdin should the logs also be sent to the standard output
	 \param verbose should verbose messages be output
	 */
	Log(const std::string & filePath, const bool logToStdin, const bool verbose = false);
	
	/** Set the verbosity level.
	 \param verbose toggle verbosity
	 */
	void setVerbose(const bool verbose);
	
	/** Default stream operator
	 \param input the object to log
	 \return itself for chaining
	 */
	template<class T>
	Log & operator<<(const T& input){
		appendIfNeeded();
		_stream << input;
		return *this;
	}
	
	/** Domain stream operator
	 \param domain the domain to use for the next log line
	 \return itself for chaining
	 \note The domain is only applied to the incoming line.
	 */
	Log & operator<<(const LogDomain& domain);
	
	/** Modifier stream operator
	 \param modif the stream modifier
	 \return itself for chaining
	 */
	Log& operator<<(std::ostream& (*modif)(std::ostream&));
	
	/** Modifier stream operator
	 \param modif the stream modifier
	 \return itself for chaining
	 */
	Log& operator<<(std::ios_base& (*modif)(std::ios_base&));
	
	/** \name GLM types support
	 @{ */
	
	/** glm::mat4 stream operator
	 \param input the object to log
	 \return itself for chaining
	 */
	Log & operator<<(const glm::mat4& input);
	
	/** glm::mat3 stream operator
	 \param input the object to log
	 \return itself for chaining
	 */
	Log & operator<<(const glm::mat3& input);
	
	/** glm::mat2 stream operator
	 \param input the object to log
	 \return itself for chaining
	 */
	Log & operator<<(const glm::mat2& input);
	
	/** glm::vec4 stream operator
	 \param input the object to log
	 \return itself for chaining
	 */
	Log & operator<<(const glm::vec4& input);
	
	/** glm::vec3 stream operator
	 \param input the object to log
	 \return itself for chaining
	 */
	Log & operator<<(const glm::vec3& input);
	
	/** glm::vec2 stream operator
	 \param input the object to log
	 \return itself for chaining
	 */
	Log & operator<<(const glm::vec2& input);
	/** @} */
	
public:
	
	/** \name Default logger
	 @{ */
	
	/** Set the default logger output file.
	 \param filePath the path to the output file
	 */
	static void setDefaultFile(const std::string & filePath);
	
	/** Set the default logger verbosity.
	 \param verbose toggle verbosity
	 */
	static void setDefaultVerbose(const bool verbose);
	
	/** The default logger with an "Info" level.
	 \return itself for chaining
	 */
	static Log& Info();
	
	/** The default logger with a "Warning" level.
	 \return itself for chaining
	 */
	static Log& Warning();
	
	/** The default logger with an "Error" level.
	 \return itself for chaining
	 */
	static Log& Error();
	
	/** @} */
	
private:
	
	/** Change the output log file.
	 \param filePath the file to write the logs to
	 \param flushExisting should the existing unwritten messages be flushed
	 */
	void setFile(const std::string & filePath, const bool flushExisting = true);
	
	/** Flush the log stream.
	 */
	void flush();

	/** Append the current domain/level prefix if it is needed.
	 */
	void appendIfNeeded();
	
	LogLevel _level; ///< The current criticality level.
	bool _logToStdOut; ///< Should the logs be output to standard output.
	std::ofstream _file; ///< The output log file stream.
	std::stringstream _stream; ///< Internal log string stream.
	bool _verbose; ///< Is the logger verbose.
	bool _ignoreUntilFlush; ///< Internal flag to ignore the current line if it is verbose.
	bool _appendPrefix; ///< Should a domain or level prefix be appended to the current line.
	
	static Log* _defaultLogger; ///< Default static logger.
};

#endif
