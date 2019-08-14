#pragma once

#include "Common.hpp"

/**
 \brief Represents a group of shaders used for rendering.
 \details Internally responsible for handling uniforms locations, shaders reloading and values caching.
 \ingroup Graphics
 */
class Program {
public:
	
	/** Default constructor */
	Program();
	
	/**
	 Load, compile and link shaders into an OpenGL program.
	 \param vertexName the name of the vertex shader
	 \param fragmentName the name of the fragment shader
	 \param geometryName the name of the geometry shader (can be empty)
	 */
	Program(const std::string & vertexName, const std::string & fragmentName, const std::string & geometryName);
	
	/** Query the location of a given uniform.
	 \param name the uniform name
	 \return the uniform location in the program
	 \note If the uniform is not present, will return -1, conviniently ignored by glUniform(...) calls.
	 */

	void uniform(const std::string & name, bool t) const;
	
	void uniform(const std::string & name, int t) const;
	
	void uniform(const std::string & name, float t) const;
	
	void uniform(const std::string & name, size_t count, const float * t) const;
	
	void uniform(const std::string & name, const glm::vec2 & t) const;
	
	void uniform(const std::string & name, const glm::vec3 & t) const;
	
	void uniform(const std::string & name, const glm::vec4 & t) const;
	
	void uniform(const std::string & name, const glm::mat3 & t) const;
	
	void uniform(const std::string & name, const glm::mat4 & t) const;

	
	
	/** Cache the values passed for the uniform array.
	 \param name the uniform array name
	 \param vals the values to cache and to set the uniform to
	 \note Other types will be added when needed.
	 */
	void cacheUniformArray(const std::string & name, const std::vector<glm::vec3> & vals);
	
	/**
	 Reload the program, recompiling the program and restoring all locations and cached uniforms.
	 */
	void reload();
	
	/** Perform full program validation and log the results.
	 \note Depending on the driver and GPU, some performance hints can be output.
	 */
	void validate();

	/**
	 Save the program to a compiled set of instructions on disk.
	 \param outputPath the output path
	 \warning The export to binary is not supported by all GPUs and will silently fail.
	 */
	void saveBinary(const std::string & outputPath);

	/** Activate the program shaders.
	 */
	void use() const;
	
	/** Delete the program. */
	void clean();
	
private:
	
	GLuint _id; ///< The OpenGL program ID.
	std::string _vertexName; ///< The vertex shader filename
	std::string _fragmentName; ///< The fragment shader filename
	std::string _geometryName; ///< The geometry shader filename
	std::map<std::string, GLint> _uniforms; ///< The list of automatically registered uniforms and their locations.
	std::map<std::string, glm::vec3> _vec3s; ///< Internal vec3 uniforms cache, for reloading.
	
};
