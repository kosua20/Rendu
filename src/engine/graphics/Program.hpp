#pragma once

#include "Common.hpp"

#include "graphics/GPUTypes.hpp"
#include "resources/Buffer.hpp"
#include "resources/Texture.hpp"

#include <unordered_map>
#include <array>

// Forward declarations
VK_DEFINE_HANDLE(VkBuffer)
VK_DEFINE_HANDLE(VkImageView)
VK_DEFINE_HANDLE(VkSampler)
VK_DEFINE_HANDLE(VkShaderModule)
VK_DEFINE_HANDLE(VkPipelineLayout)
VK_DEFINE_HANDLE(VkDescriptorSetLayout)

#define UNIFORMS_SET 0
#define SAMPLERS_SET 1
#define IMAGES_SET 2
#define BUFFERS_SET 3

/**
 \brief Represents a group of shaders used for rendering.
 \details Internally responsible for handling uniforms locations, shaders reloading and values caching.
 Uniform sets are predefined: set 0 is for dynamic uniforms, set 1 for image-samplers, set 2 for static/per-frame uniform buffers.
 \ingroup Graphics
 */
class Program {
public:

	/** Bind all mip levels of a texture */
	static uint ALL_MIPS;

	/** Type of program */
	enum class Type {
		GRAPHICS, ///< Graphics program for draw calls.
		COMPUTE ///< Compute program for dispatch calls.
	};

	/** \brief Uniform reflection information.
	 */
	struct UniformDef {

		/// Uniform basic type.
		enum class Type {
			BOOL, BVEC2, BVEC3, BVEC4,
			INT, IVEC2, IVEC3, IVEC4,
			UINT, UVEC2, UVEC3, UVEC4,
			FLOAT, VEC2, VEC3, VEC4,
			MAT2, MAT3, MAT4,
			OTHER
		};

		std::string name; ///< The uniform name.
		Type type; ///< The uniform type.

		/// Uniform location.
		struct Location {
			uint binding; ///< Buffer binding.
			uint offset; ///< Offset in buffer.
		};

		std::vector<Location> locations; ///< Locations where this uniform is present.

	};

	/** \brief Image-sampler reflection information.
	 */
	struct ImageDef {
		std::string name; ///< Image name.
		TextureShape shape; ///< Image shape.
		uint binding; ///< Image binding location.
		uint set; ///< Image binding set.
		bool storage; ///< Is this a storage image.
	};

	/** \brief Buffer reflection information.
	 */
	struct BufferDef {
		std::vector<UniformDef> members; ///< Uniforms in buffer.
		std::string name; ///< Buffer name.
		uint binding; ///< Buffer binding location.
		uint size; ///< Buffer size.
		uint set; ///< Buffer binding set.
		bool storage; ///< Is this a storage buffer.
	};

	using Uniforms = std::unordered_map<std::string, UniformDef>; ///< List of named uniforms.

	/**
	 Load, compile and link shaders into a GPU graphics program.
	 \param name the program name for logging
	 \param vertexContent the content of the vertex shader
	 \param fragmentContent the content of the fragment shader
	 \param tessControlContent the content of the tessellation control shader (can be empty)
	 \param tessEvalContent the content of the tessellation evaluation shader (can be empty)
	 */
	Program(const std::string & name, const std::string & vertexContent, const std::string & fragmentContent, const std::string & tessControlContent = "", const std::string & tessEvalContent = "");

	/**
	 Load, compile and link shaders into a GPU compute program.
	 \param name the program name for logging
	 \param computeContent the content of the compute shader
	 */
	Program(const std::string & name, const std::string & computeContent);

	/**
	 Load the program, compiling the shader and updating all uniform locations.
	 \param vertexContent the content of the vertex shader
	 \param fragmentContent the content of the fragment shader
	 \param tessControlContent the content of the tessellation control shader (can be empty)
	 \param tessEvalContent the content of the tessellation evaluation shader (can be empty)
	 */
	void reload(const std::string & vertexContent, const std::string & fragmentContent, const std::string & tessControlContent = "", const std::string & tessEvalContent = "");

	/**
	 Load the program, compiling the shader and updating all uniform locations.
	 \param computeContent the content of the compute shader
	 */
	void reload(const std::string & computeContent);

	/** \return true if the program has been recently reloaded. */
	bool reloaded() const;
	
	/** Check if the program has been recently reloaded.
	 * \param absorb should the reloaded flag be set to false afterwards
	 * \return true if reloaded
	 */
	bool reloaded(bool absorb);

	/** Activate the program shaders.
	 */
	void use() const;

	/** Delete the program on the GPU.
	 */
	void clean();

	/** Bind a buffer to a given location.
	 * \param buffer the buffer to bind
	 * \param slot the location to bind to
	 */
	void buffer(const UniformBufferBase& buffer, uint slot);

	/** Bind a buffer to a given location.
	 * \param buffer the buffer to bind
	 * \param slot the location to bind to
	 */
	void buffer(const Buffer& buffer, uint slot);

	/** Bind a texture to a given location.
	 * \param texture the texture to bind
	 * \param slot the location to bind to
	 * \param mip the mip of the texture to bind (or all mips if left at its default value)
	 */
	void texture(const Texture* texture, uint slot, uint mip = Program::ALL_MIPS);

	/** Bind a texture to a given location.
	 * \param texture the texture to bind
	 * \param slot the location to bind to
	 * \param mip the mip of the texture to bind (or all mips if left at its default value)
	 */
	void texture(const Texture& texture, uint slot, uint mip = Program::ALL_MIPS);

	/** Bind a set of textures to successive locations.
	 * \param textures the textures to bind
	 * \param slot the location to bind the first texture to
	 * \note Successive textures will be bound to locations slot+1, slot+2,...
	 */
	void textures(const std::vector<const Texture *> & textures, size_t slot = 0);

	/** Bind a default texture to a given location.
	 * \param slot the location to bind the texture to
	 */
	void defaultTexture(uint slot);

	/** Update internal data (descriptors,...) and bind them before a draw/dispatch. */
	void update();

	/** Set a given uniform value.
	 \param name the uniform name
	 \param t the value to set the uniform to
	 */
	void uniform(const std::string & name, bool t);

	/** Set a given uniform value.
	 \param name the uniform name
	 \param t the value to set the uniform to
	 */
	void uniform(const std::string & name, int t);

	/** Set a given uniform value.
	 \param name the uniform name
	 \param t the value to set the uniform to
	 */
	void uniform(const std::string & name, uint t);

	/** Set a given uniform value.
	 \param name the uniform name
	 \param t the value to set the uniform to
	 */
	void uniform(const std::string & name, float t);

	/** Set a given uniform value.
	 \param name the uniform name
	 \param t the value to set the uniform to
	 */
	void uniform(const std::string & name, const glm::vec2 & t);

	/** Set a given uniform value.
	 \param name the uniform name
	 \param t the value to set the uniform to
	 */
	void uniform(const std::string & name, const glm::vec3 & t);

	/** Set a given uniform value.
	 \param name the uniform name
	 \param t the value to set the uniform to
	 */
	void uniform(const std::string & name, const glm::vec4 & t);

	/** Set a given uniform value.
	 \param name the uniform name
	 \param t the value to set the uniform to
	 */
	void uniform(const std::string & name, const glm::ivec2 & t);

	/** Set a given uniform value.
	 \param name the uniform name
	 \param t the value to set the uniform to
	 */
	void uniform(const std::string & name, const glm::ivec3 & t);

	/** Set a given uniform value.
	 \param name the uniform name
	 \param t the value to set the uniform to
	 */
	void uniform(const std::string & name, const glm::ivec4 & t);

	/** Set a given uniform value.
	 \param name the uniform name
	 \param t the value to set the uniform to
	 */
	void uniform(const std::string & name, const glm::mat3 & t);

	/** Set a given uniform value.
	 \param name the uniform name
	 \param t the value to set the uniform to
	 */
	void uniform(const std::string & name, const glm::mat4 & t);

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
	const Uniforms & uniforms() const {
		return _uniforms;
	}

	/** \return the program name */
	const std::string & name() const {
		return _name;
	}

	/** \return whether this is a graphics or compute program */
	Program::Type type() const {
		return _type;
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
	Program & operator=(Program &&) = delete;
	
	/** Move constructor. */
	Program(Program &&) = default;

	/// \brief Program pipeline state.
	struct State {
		std::vector<VkDescriptorSetLayout> setLayouts; ///< Descriptor sets layouts.
		VkPipelineLayout layout = VK_NULL_HANDLE; ///< Layout handle (pre-created).
	};

	/// \return the program state for a pipeline
	const State& getState() const {
		return _state;
	}

	/// \brief Per-stage reflection information.
	struct Stage {
		std::vector<ImageDef> images; ///< Image definitions.
		std::vector<BufferDef> buffers; ///< Buffers definitions.
		VkShaderModule module = VK_NULL_HANDLE; ///< Native shader data.

		/// Reset the stage state.
		void reset();
	};

	/** Query shader information for a stage.
	 * \param type the stage to query
	 * \return the stage reflection information
	 */
	Stage& stage(ShaderType type){
		return _stages[uint(type)];
	}

private:

	/// Reflect all uniforms/textures/storage buffers and images based on the shader content.
	void reflect();


	/// Update internal metrics.
	void updateUniformMetric() const;

	/** Obtain a pointer to the CPU memory for a uniform.
	 * \param location the uniform location to retrieve
	 * \return a const pointer to CPU memory
	 */ 
	inline const char* retrieveUniform(const UniformDef::Location& location) const {
		return &(_dynamicBuffers.at(location.binding).buffer->data[location.offset]);
	}

	/** Obtain a pointer to the CPU memory for a uniform.
	 * \param location the uniform location to retrieve
	 * \return a pointer to CPU memory
	 * \note This will mark the corresponding buffer as dirty.
	 */ 
	inline char* retrieveUniformNonConst(const UniformDef::Location& location) {
		DynamicBufferState& buffState = _dynamicBuffers.at(location.binding);
		buffState.dirty = true;
		_dirtySets[0] = true;
		return &(buffState.buffer->data[location.offset]);
	}

	/// \brief Internal state for a dynamic uniform buffer.
	struct DynamicBufferState {
		std::shared_ptr<UniformBuffer<char>> buffer; ///< Owned Uniform buffer.
		uint descriptorIndex = 0; ///< Descriptor index in set.
		bool dirty = true; ///< Is the buffer dirty since last draw.
	};

	/// \brief Internal state for an image-sampler.
	struct TextureState {
		std::string name; ///< Name.
		TextureShape shape = TextureShape::D2; ///< Texture shape.
		const Texture* texture; ///< The source texture.
		VkImageView view = VK_NULL_HANDLE; ///< Texture view.
		uint mip = 0xFFFF; ///< The corresponding mip.
		bool storage = false; ///< Is the image used as storage.
	};

	/// \brief Internal state for a static (external) uniform buffer.
	struct StaticBufferState {
		std::string name; ///< Name.
		VkBuffer buffer = VK_NULL_HANDLE; ///< Native buffer handle.
		uint offset = 0; ///< Start offset in the buffer.
		uint size = 0; ///< Region size in the buffer.
		bool storage = false; ///< Is the buffer used as storage.
	};


	std::string _name; ///< Debug name.
	std::array<Stage, int(ShaderType::COUNT)> _stages; ///< Per-stage reflection data.
	State _state; ///< Program pipeline state.

	std::unordered_map<std::string, UniformDef> _uniforms; ///< All dynamic uniform definitions.

	std::unordered_map<int, DynamicBufferState> _dynamicBuffers; ///< Dynamic uniform buffer definitions (set 0).
	std::unordered_map<int, TextureState> _textures; ///< Dynamic image-sampler definitions (set 2).
	std::unordered_map<int, StaticBufferState> _staticBuffers; ///< Static uniform buffer definitions (set 3).

	std::array<bool, 4> _dirtySets; ///< Marks which descriptor sets are dirty.
	std::array<DescriptorSet, 4> _currentSets; ///< Descriptor sets.
	std::vector<uint32_t> _currentOffsets; ///< Offsets in the descriptor set for dynamic uniform buffers.

	bool _reloaded = false; ///< Has the program been reloaded.
	const Type _type; ///< Is this a compute shader.

	friend class GPU; ///< Utilities will need to access GPU handle.
};
