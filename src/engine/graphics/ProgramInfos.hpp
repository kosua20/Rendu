#ifndef ProgramInfos_h
#define ProgramInfos_h

#include "../Common.hpp"
#include <map>

/**
 \brief Represents a group of shaders used for rendering.
 \details Internally responsible for handling uniforms locations, shaders reloading and values caching.
 \ingroup Helpers
 */
class ProgramInfos {
public:
	
	/** Default constructor */
	ProgramInfos();
	
	/**
	 Load, compile and link shaders into an OpenGL program.
	 \param vertexName the name of the vertex shader
	 \param fragmentName the name of the fragment shader
	 \param geometryName the name of the geometry shader (can be empty)
	 */
	ProgramInfos(const std::string & vertexName, const std::string & fragmentName, const std::string & geometryName);
	
	/** Destructor */
	~ProgramInfos();
	
	/** Query the location of a given uniform.
	 \param name the uniform name
	 \return the uniform location in the program
	 \note If the uniform is not present, will return -1, conviniently ignored by glUniform(...) calls.
	 */
	const GLint uniform(const std::string & name) const;

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

	/** Query the program ID.
	 \return the OpenGL ID
	 */
	const GLuint id() const { return _id; }
	
private:
	
	GLuint _id; ///< The OpenGL program ID.
	std::string _vertexName; ///< The vertex shader filename
	std::string _fragmentName; ///< The fragment shader filename
	std::string _geometryName; ///< The geometry shader filename
	std::map<std::string, GLint> _uniforms; ///< The list of automatically registered uniforms and their locations.
	std::map<std::string, glm::vec3> _vec3s; ///< Internal vec3 uniforms cache, for reloading.
	
};



#endif
