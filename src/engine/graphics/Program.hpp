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

	/** Uniform reflection information.
	 Note that GL info are stored separately internally.
	 */
	struct Uniform {

		/// Uniform basic type.
		enum class Type {
			BOOL, BVEC2, BVEC3, BVEC4,
			INT, IVEC2, IVEC3, IVEC4,
			UINT, UVEC2, UVEC3, UVEC4,
			FLOAT, VEC2, VEC3, VEC4,
			MAT2, MAT3, MAT4,
			OTHER
		};

		/** Constructor.
		 \param uname uniform name
		 \param utype uniform type
		 */
		Uniform(const std::string & uname, Type utype);

		std::string name; ///< The uniform name.
		Type type; ///< The uniform type.
	};


	/**
	 Load, compile and link shaders into an OpenGL program.
	 \param name the program name for logging
	 \param vertexContent the content of the vertex shader
	 \param fragmentContent the content of the fragment shader
	 \param geometryContent the content of the geometry shader (can be empty)
	 \param tessControlContent the content of the tessellation control shader (can be empty)
	 \param tessEvalContent the content of the tessellation evaluation shader (can be empty)
	 */
	Program(const std::string & name, const std::string & vertexContent, const std::string & fragmentContent, const std::string & geometryContent = "", const std::string & tessControlContent = "", const std::string & tessEvalContent = "");

	/**
	 Load the program, compiling the shader and updating all uniform locations.
	 \param vertexContent the content of the vertex shader
	 \param fragmentContent the content of the fragment shader
	 \param geometryContent the content of the geometry shader (can be empty)
	 \param tessControlContent the content of the tessellation control shader (can be empty)
	 \param tessEvalContent the content of the tessellation evaluation shader (can be empty)
	 */
	void reload(const std::string & vertexContent, const std::string & fragmentContent, const std::string & geometryContent, const std::string & tessControlContent = "", const std::string & tessEvalContent = "");

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
	void uniform(const std::string & name, uint t) const;

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
	void uniform(const std::string & name, const glm::ivec2 & t) const;

	/** Set a given uniform value.
	 \param name the uniform name
	 \param t the value to set the uniform to
	 */
	void uniform(const std::string & name, const glm::ivec3 & t) const;

	/** Set a given uniform value.
	 \param name the uniform name
	 \param t the value to set the uniform to
	 */
	void uniform(const std::string & name, const glm::ivec4 & t) const;

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

	/** Get a given uniform value.
	 \param name the uniform name
	 \param t will contain the value
	 */
	void getUniform(const std::string & name, bool & t) const;

	/** Get a given uniform value.
	\param name the uniform name
	\param t will contain the value
	*/
	void getUniform(const std::string & name, int & t) const;

	/** Get a given uniform value.
	\param name the uniform name
	\param t will contain the value
	*/
	void getUniform(const std::string & name, uint & t) const;

	/** Get a given uniform value.
	\param name the uniform name
	\param t will contain the value
	*/
	void getUniform(const std::string & name, float & t) const;

	/** Get a given uniform value.
	\param name the uniform name
	\param t will contain the value
	*/
	void getUniform(const std::string & name, glm::vec2 & t) const;

	/** Get a given uniform value.
	\param name the uniform name
	\param t will contain the value
	*/
	void getUniform(const std::string & name, glm::vec3 & t) const;

	/** Get a given uniform value.
	\param name the uniform name
	\param t will contain the value
	*/
	void getUniform(const std::string & name, glm::vec4 & t) const;

	/** Get a given uniform value.
	\param name the uniform name
	\param t will contain the value
	*/
	void getUniform(const std::string & name, glm::ivec2 & t) const;

	/** Get a given uniform value.
	\param name the uniform name
	\param t will contain the value
	*/
	void getUniform(const std::string & name, glm::ivec3 & t) const;

	/** Get a given uniform value.
	\param name the uniform name
	\param t will contain the value
	*/
	void getUniform(const std::string & name, glm::ivec4 & t) const;

	/** Get a given uniform value.
	\param name the uniform name
	\param t will contain the value
	*/
	void getUniform(const std::string & name, glm::mat3 & t) const;

	/** Get a given uniform value.
	\param name the uniform name
	\param t will contain the value
	*/
	void getUniform(const std::string & name, glm::mat4 & t) const;

	/** \return the list of registered basic uniforms.
	 */
	const std::vector<Uniform> & uniforms() const {
		return _uniformInfos;
	}

	/** \return the program name */
	const std::string & name() const {
		return _name;
	}

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
	std::map<std::string, GLint> _uniforms;  ///< Internal list of automatically registered uniforms and their locations. We keep this separate to avoid exposing GL internal types.
	std::vector<Uniform> _uniformInfos;  ///< Additional uniforms info.

	friend class GLUtilities;
};
