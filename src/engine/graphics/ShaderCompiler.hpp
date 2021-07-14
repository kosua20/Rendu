#pragma once

#include "graphics/Program.hpp"
#include "graphics/GPUObjects.hpp"
#include "Common.hpp"

// Forward declarations
namespace glslang {
	class TProgram;
	class TType;
}

/**
 \brief 
 \ingroup Graphics
 */
class ShaderCompiler {

public:

	/** Create a shader of a given type from a string. Extract additional informations from the shader.
	 \param prog the content of the shader
	 \param type the type of shader (vertex, fragment,...)
	 \param bindings will be filled with the samplers/buffers present in the shader and their user-defined locations
	 \param finalLog will contain the compilation log of the shader
	 \return the GPU ID of the shader object
	 */
	static void compile(const std::string & prog, ShaderType type, Program::Stage & stage, std::string & finalLog);

	static bool init();

	static void cleanup();
	
private:

	static Program::UniformDef::Type convertType(const glslang::TType& type);

	static uint getSetFromType(const glslang::TType& type);

	static void reflect(glslang::TProgram & program, Program::Stage & stage);

};
