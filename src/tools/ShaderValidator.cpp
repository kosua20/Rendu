#include "resources/ResourcesManager.hpp"
#include "graphics/GLUtilities.hpp"
#include "system/Window.hpp"
#include "Common.hpp"
#include <iostream>
#include <map>
#include <sstream>

#include "system/TextUtilities.hpp"

/**
 \defgroup ShaderValidator Shader Validation
 \brief Validate shaders compilation on the GPU and output IDE-compliant error messages.
 \ingroup Tools
 */

/**  Convert a shader compilation log into a IDE-compatible error reporting format and output it to stderr.
 	\param compilationLog the compilation log to process.
 	\param filePaths the paths to the shader file and all include files, absolute or relative to the directory containing the IDE project.
 	\return a boolean denoting if at least one error was reported by the log.
 	\warning If filePath is not expressed absolute or relative to the directory containing the IDE project, error links (for instance "src/foo/bar.frag:18") won't be functional.
 	\ingroup ShaderValidator
 */
bool processLog(const std::string & compilationLog, const std::vector<std::string> & filePaths) {
	if(!compilationLog.empty()) {
		std::stringstream str(compilationLog);
		std::string line;
		// Iterate over the lines of the log.
		while(std::getline(str, line)) {
			// Parse the log and output it as a compiler readable error.
			// Find the global file ID limits.
			const std::string::size_type firstIDDigitPos = line.find_first_of("0123456789");
			const std::string::size_type lastIDDigitPos  = line.find_first_not_of("0123456789", firstIDDigitPos);
			if(firstIDDigitPos == std::string::npos || lastIDDigitPos == std::string::npos) {
				continue;
			}
			// Find the line number limits.
			const std::string::size_type firstLineDigitPos = line.find_first_of("0123456789", lastIDDigitPos + 1);
			const std::string::size_type lastLineDigitPos  = line.find_first_not_of("0123456789", firstLineDigitPos);
			if(firstLineDigitPos == std::string::npos || lastLineDigitPos == std::string::npos) {
				continue;
			}
			// Find the file containing the error based on the ID.
			const std::string fileNumberRaw = line.substr(firstIDDigitPos, lastIDDigitPos - 1 - firstIDDigitPos + 1);
			const unsigned int fileId = std::stoi(fileNumberRaw);
			// Update the file path.
			std::string finalFilePath = "unknown_file";
			if(fileId < filePaths.size()) {
				finalFilePath = filePaths[fileId];
			}
			// Generate the corresponding int.
			const std::string lineNumberRaw = line.substr(firstLineDigitPos, lastLineDigitPos - 1 - firstLineDigitPos + 1);
			const unsigned int lineId		= std::stoi(lineNumberRaw);

			// Find the error message.
			const std::string::size_type firstMessagePos = line.find_first_not_of(" :)]", lastLineDigitPos + 1);
			std::string errorMessage					 = "Unknown error.";
			if(firstMessagePos != std::string::npos) {
				errorMessage = line.substr(firstMessagePos);
			}

			// The path should be relative to the root build directory.
			// Output in an IDE compatible format, to display warning and errors properly.
#ifdef _WIN32
			std::cerr << finalFilePath << "(" << lineId << "): error: " << errorMessage << std::endl;
#else
			std::cerr << finalFilePath << ":" << lineId << ": error: " << errorMessage << std::endl;
#endif
		}
		// At least one issue was encountered.
		return true;
	}
	// No log, no problem.
	return false;
}

/**
 Perform shader validation: load all shaders in the resources directory, compile them on the GPU and output error logs.
 \param argc the number of input arguments.
 \param argv a pointer to the raw input arguments.
 \return a boolean denoting if at least one shader failed to compile.
 \ingroup ShaderValidator
 */
int main(int argc, char ** argv) {

	Log::setDefaultVerbose(false);
	if(argc < 2) {
		Log::Error() << "Missing resource path." << std::endl;
		return 1;
	}
	Resources::manager().addResources(std::string(argv[1]));
	
	RenderingConfig config({ "ShaderValidator", "wxh", "100", "100"});
	Window window("Validation", config, false, true);

	// Query the renderer identifier, and the supported OpenGL version.
	std::string vendor, renderer, version, shaderVersion;
	GLUtilities::deviceInfos(vendor, renderer, version, shaderVersion);
	Log::Info() << Log::OpenGL << "Vendor: " << vendor << "." << std::endl;
	Log::Info() << Log::OpenGL << "Internal renderer: " << renderer << "." << std::endl;
	Log::Info() << Log::OpenGL << "Versions: Driver: " << version << ", GLSL: " << shaderVersion << "." << std::endl;

	// We will need all glsl files for include support.
	std::map<std::string, std::string> includeFiles;
	Resources::manager().getFiles("glsl", includeFiles);

	// Test all shaders.
	const std::map<ShaderType, std::string> types = {
		{ShaderType::VERTEX, "vert" },
		{ShaderType::GEOMETRY, "geom" },
		{ShaderType::FRAGMENT, "frag" },
		{ShaderType::TESSCONTROL, "tessc" },
		{ShaderType::TESSEVAL, "tesse" }
	};
	bool encounteredIssues = false;
	
	for(const auto & type : types) {
		// Load shaders from disk.
		std::map<std::string, std::string> files;
		Resources::manager().getFiles(type.second, files);
		for (auto& file : files) {
			GLUtilities::Bindings bindings;
			std::string compilationLog;
			// Keep track of the include files used.
			// File with ID 0 is the base file, already set its name.
			std::vector<std::string> names = { file.second };
			// Load the shader.
			const std::string fullName = file.first + "." + type.second;
			const std::string shader = Resources::manager().getStringWithIncludes(fullName, names);
			// Compile the shader.
			GLUtilities::loadShader(shader, type.first, bindings, compilationLog);
			// Replace the include names by the full paths.
			for(size_t nid = 1; nid < names.size(); ++nid) {
				auto& name = names[nid];
				TextUtilities::splitExtension(name);
				if(includeFiles.count(name) > 0) {
					name = includeFiles[name];
				}
			}
			// Process the log.
			const bool newIssues = processLog(compilationLog, names);
			encounteredIssues = encounteredIssues || newIssues;
		}
	}

	// Has any of the shaders encountered a compilation issue?
	return encounteredIssues ? 1 : 0;
}
