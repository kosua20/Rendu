#pragma once

#include "Common.hpp"
#include <map>


/**
 \brief Represents a group of shaders used for rendering.
 \details Internally responsible for handling uniforms locations, shaders reloading and values caching.
 \ingroup Graphics
 */
class Program {
public:

	/**
	 Load, compile and link shaders into an OpenGL program.
	 \param name the program name for logging
	 \param vertexContent the content of the vertex shader
	 \param fragmentContent the content of the fragment shader
	 \param geometryContent the content of the geometry shader (can be empty)
	 */
	Program(const std::string & name, const std::string & vertexContent, const std::string & fragmentContent, const std::string & geometryContent);

	/**
	 Load the program, compiling the shader and updating all uniform locations.
	 \param vertexContent the content of the vertex shader
	 \param fragmentContent the content of the fragment shader
	 \param geometryContent the content of the geometry shader (can be empty)
	 */
	void reload(const std::string & vertexContent, const std::string & fragmentContent, const std::string & geometryContent);

	/** Perform full program validation and log the results.
	 \note Depending on the driver and GPU, some performance hints can be output.
	 */
	void validate() const;

	/**
	 Save the program to a compiled set of instructions on disk.
	 \param outputPath the output path
	 \warning The export to binary is not supported by all GPUs and will silently fail.
	 */
	void saveBinary(const std::string & outputPath) const;

	/** Activate the program shaders.
	 */
	void use() const;

	/** Delete the program on the GPU.
	 */
	void clean() const;

	/** Set a given uniform value.
	 \param name the uniform name
	 \param t the value to set the uniform to
	 */
	void uniform(const std::string & name, bool t) const;

	/** Set a given uniform value.
	 \param name the uniform name
	 \param t the value to set the uniform to
	 */
	void uniform(const std::string & name, int t) const;

	/** Set a given uniform value.
	 \param name the uniform name
	 \param t the value to set the uniform to
	 */
	void uniform(const std::string & name, float t) const;

	/** Set a given float array uniform values.
	 \param name the uniform name (including "[0]")
	 \param count the number of values in the float array
	 \param t the values to set the uniform to
	 */
	void uniform(const std::string & name, size_t count, const float * t) const;

	/** Set a given uniform value.
	 \param name the uniform name
	 \param t the value to set the uniform to
	 */
	void uniform(const std::string & name, const glm::vec2 & t) const;

	/** Set a given uniform value.
	 \param name the uniform name
	 \param t the value to set the uniform to
	 */
	void uniform(const std::string & name, const glm::vec3 & t) const;

	/** Set a given uniform value.
	 \param name the uniform name
	 \param t the value to set the uniform to
	 */
	void uniform(const std::string & name, const glm::vec4 & t) const;

	/** Set a given uniform value.
	 \param name the uniform name
	 \param t the value to set the uniform to
	 */
	void uniform(const std::string & name, const glm::mat3 & t) const;

	/** Set a given uniform value.
	 \param name the uniform name
	 \param t the value to set the uniform to
	 */
	void uniform(const std::string & name, const glm::mat4 & t) const;

	/** Set a given uniform buffer binding point.
	 \param name the uniform name
	 \param slot the binding point
	 */
	void uniformBuffer(const std::string & name, size_t slot) const;

	/** Set a given uniform sampler binding point.
	 \param name the uniform name
	 \param slot the binding point
	 */
	void uniformTexture(const std::string & name, size_t slot) const;

	/** Copy assignment operator (disabled).
	 \return a reference to the object assigned to
	 */
	Program & operator=(const Program &) = delete;
	
	/** Copy constructor (disabled). */
	Program(const Program &) = delete;
	
	/** Move assignment operator.
	 \return a reference to the object assigned to
	 */
	Program & operator=(Program &&) = default;
	
	/** Move constructor. */
	Program(Program &&) = default;
	
private:

	GLuint _id;								 ///< The OpenGL program ID.
	std::string _name;				 		 ///< The shader name
	std::map<std::string, GLint> _uniforms;  ///< The list of automatically registered uniforms and their locations.

};
