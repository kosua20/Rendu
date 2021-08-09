#pragma once

#include "Common.hpp"
#include <map>

#include "graphics/GPUObjects.hpp"
#include "resources/Buffer.hpp"
#include "resources/Texture.hpp"

#include <volk/volk.h>

/**
 \brief Represents a group of shaders used for rendering.
 \details Internally responsible for handling uniforms locations, shaders reloading and values caching.
 \ingroup Graphics
 */
class Program {
public:

	/** Uniform reflection information.
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

		struct Location {
			uint binding;
			uint offset;
		};

		std::vector<Location> locations;

	};

	using Uniforms = std::unordered_map<std::string, UniformDef>;

	struct SamplerDef {
		std::string name;
		TextureShape shape;
		uint binding;
		uint set;
	};

	struct BufferDef {
		std::string name;
		uint binding;
		uint size;
		uint set;
		std::vector<UniformDef> members;
	};

	struct State {
		std::vector<VkPipelineShaderStageCreateInfo> stages;
		std::vector<VkDescriptorSetLayout> setLayouts;
		VkPipelineLayout layout = VK_NULL_HANDLE;
	};

	/**
	 Load, compile and link shaders into a GPU program.
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

	bool reloaded() const;
	
	bool reloaded(bool absorb);

	/** Activate the program shaders.
	 */
	void use() const;

	/** Delete the program on the GPU.
	 */
	void clean();

	void buffer(const UniformBufferBase& buffer, uint slot);

	void texture(const Texture* texture, uint slot, uint mip = 0xFFFF);

	void texture(const Texture& texture, uint slot, uint mip = 0xFFFF);

	void textures(const std::vector<const Texture *> & textures, size_t startingSlot = 0);

	void defaultTexture(uint slot);
	
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

	/** Set a given uniform buffer binding point.
	 \param name the uniform name
	 \param slot the binding point
	 */
	//void uniformBuffer(const std::string & name, size_t slot) const;

	/** Set a given uniform sampler binding point.
	 \param name the uniform name
	 \param slot the binding point
	 */
	//void uniformTexture(const std::string & name, size_t slot) const;

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

	const State& getState() const {
		return _state;
	}

	struct Stage {
		std::vector<SamplerDef> samplers;
		std::vector<BufferDef> buffers;
		VkShaderModule module = VK_NULL_HANDLE;

		void reset(){
			samplers.clear();
			buffers.clear();
			module = VK_NULL_HANDLE;
		}
	};

	Stage& stage(ShaderType type){
		return _stages[uint(type)];
	}

private:

	void updateUniformMetric() const; ///< Update internal metrics.

	inline const char* retrieveUniform(const UniformDef::Location& location) const {
		return &(_dynamicBuffers.at(location.binding).buffer->data[location.offset]);
	}

	inline char* retrieveUniformNonConst(const UniformDef::Location& location) {
		DynamicBufferState& buffState = _dynamicBuffers.at(location.binding);
		buffState.dirty = true;
		_dirtySets[0] = true;
		return &(buffState.buffer->data[location.offset]);
	}

	std::array<Stage, int(ShaderType::COUNT)> _stages;

	std::string _name;
	State _state;

	struct DynamicBufferState {
		std::shared_ptr<UniformBuffer<char>> buffer;
		uint descriptorIndex = 0;
		bool dirty = true;
	};

	struct TextureState {
		std::string name;
		VkImageView view = VK_NULL_HANDLE;
		VkSampler sampler = VK_NULL_HANDLE;
		TextureShape shape = TextureShape::D2;
	};

	struct StaticBufferState {
		std::string name;
		VkBuffer buffer = VK_NULL_HANDLE;
		uint offset = 0;
		uint size = 0;
	};

	std::unordered_map<std::string, UniformDef> _uniforms;

	std::unordered_map<int, DynamicBufferState> _dynamicBuffers; // set 0
	std::unordered_map<int, TextureState> _textures; // set 1
	std::unordered_map<int, StaticBufferState> _staticBuffers; // set 2

	std::array<bool, 3> _dirtySets;
	std::array<DescriptorSet, 3> _currentSets;
	std::vector<uint32_t> _currentOffsets;

	bool _reloaded = false;

	friend class GPU; ///< Utilities will need to access GPU handle.
};
