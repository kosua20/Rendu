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
 \brief Relies on glslang to compile GLSL shaders to SPIR-V and generate reflection data.
 \ingroup Graphics
 */
class ShaderCompiler {

public:

	/** Create a shader of a given type from a string. Extract additional informations from the shader.
	 \param prog the content of the shader
	 \param type the type of shader (vertex, fragment,...)
	 \param stage will be filled with reflection information: the samplers/buffers present in the shader and their user-defined locations, along with the compiled results
	 \param generateModule shoud a Vulkan module be generated from the SPIR-V code
	 \param finalLog will contain the compilation log of the shader
	 */
	static void compile(const std::string & prog, ShaderType type, Program::Stage & stage, bool generateModule, std::string & finalLog);

	/** Initialize the compiler library.
	 * \return the success status
	 */
	static bool init();

	/** Close the compiler library. */
	static void cleanup();

	/** Destroy a compiled shader
	 * \param stage the shader to cleanup
	 */
	static void clean(Program::Stage & stage);
	
private:

	/** Convert a uniform compiler type to Rendu internal types.
	 * \param type the type to convert
	 * \return the corresponding Rendu uniform type
	 */
	static Program::UniformDef::Type convertType(const glslang::TType& type);

	/** Retrieve a set location from its type.
	 * \param type the type of the set
	 * \return the set location 
	 */
	static uint getSetFromType(const glslang::TType& type);

	/** Perform reflection on a compiled program and populate our reflection structures.
	 * \param program the compiled SPIR-V program
	 * \param stage will contain reflection data
	 * */
	static void reflect(glslang::TProgram & program, Program::Stage & stage);

};
