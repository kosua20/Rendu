#pragma once

#include "Application.hpp"
#include "graphics/Framebuffer.hpp"
#include "graphics/Program.hpp"

#include "Common.hpp"

/**
 \brief Shader editor that can be used to tweak shaders on the fly, reloading them and exposing parameters for adjustements.
 \ingroup ShaderBench
 */
class ShaderEditor final : public CameraApp {

public:
	/** Constructor.
	 \param config the configuration to apply
	 */
	explicit ShaderEditor(RenderingConfig & config);

	/** \copydoc CameraApp::draw */
	void draw() override;

	/** \copydoc CameraApp::update
	 */
	void update() override;

	/** \copydoc CameraApp::resize
	 */
	void resize() override;

	/** Destructor. */
	~ShaderEditor() override;

	void loadShader(const std::string& path);

private:

	/** React to user key inputs. */
	void handleShortcuts();

	/** Display the uniform GUI elements.
	 \param columnsCount maximum number of columns to use for small elements (booleans,...)
	 */
	void displayUniforms(uint columnsCount);

	/** Reload the shader and get the potential error message.
	 \param shaderPath path to the shader to reload on disk
	 \param syncUniforms should the predefined uniforms be extracted from the shader and their values transferred to the GUI
	 \return the compilation error log or an empty string
	 */
	std::string reload(const std::string & shaderPath, bool syncUniforms);

	/** Restore the values of all predefined uniforms that are present in the shader with default values. */
	void restoreUniforms();

	/** Helper generation of a string listing all predefined tweakable uniforms.
	 \param prefix prefix to append before each element type ("uniform" or "const" for instance)
	 \param exportValues should the current values be writtent ("float f0 = 3.0;" for instance)
	 \return the generated string
	 */
	std::string generateParametersString(const std::string & prefix, bool exportValues);


	/// Boolean flag parameter.
	struct BoolOption {
		std::string name; ///< Uniform name.
		bool value = false; ///< Uniform value.
	};

	/// Integer parameter.
	struct IntOption {
		std::string name; ///< Uniform name.
		int value = 0; ///< Uniform value.
	};

	/// Float parameter.
	struct FloatOption {
		std::string name; ///< Uniform name.
		float value = 0.0f; ///< Uniform value.
		float min = 0.0f; ///< Minimum possible value.
		float max = 1.0f; ///< Maximum possible value.
	};

	/// 4D Vector parameter.
	struct VecOption {
		std::string name; ///< Uniform name.
		glm::vec4 value = glm::vec4(0.0f); ///< Uniform value.
	};

	/// RGB Color parameter.
	struct ColorOption {
		std::string name; ///< Uniform name.
		glm::vec3 value = glm::vec3(1.0f, 0.0f, 0.0f); ///< Uniform value.
	};

	/// GUI layout options.
	enum class LayoutMode : int {
		HORIZONTAL, ///< Panel to the left, result to the right.
		VERTICAL, ///< Panel at the bottom, result at the top.
		FREEFORM ///< Panels can be freely moved around.
	};

	std::unique_ptr<Framebuffer> _currFrame; ///< Content buffer.
	std::unique_ptr<Framebuffer> _prevFrame; ///< Content buffer.

	std::string _shaderPath; ///< Path of the current shader on disk.
	std::string _shaderName; ///< Name of the current shader (for display).
	std::unique_ptr<Program> _currProgram; ///< Current shader program.
	Program * _passthrough; ///< Passthrough program.
	GPUQuery _timer = GPUQuery(GPUQuery::Type::TIME_ELAPSED); ///< Timer for the user shader pass.

	std::vector<const Texture *> _textures; ///< List of all predefined textures.
	Texture _noise; ///< Random 2D RGBA uniform noise in [0,1].
	Texture _perlin; ///< Random 2D RGBA periodic Perlin noise in [0,1].
	Texture _directions; ///< Random 3D directions on the sphere.
	Texture _noise3D; ///< Random 3D RGBA uniform noise in [0,1].
	Texture _perlin3D; ///< Random 3D RGBA periodic Perlin noise in [0,1].
	const Texture * _fallbackTex = nullptr; ///< Display texture for non-2D inputs.

	std::vector<BoolOption> _flags; ///< Predefined boolean parameters.
	std::vector<IntOption> _integers; ///< Predefined integer parameters.
	std::vector<FloatOption> _floats; ///< Predefined float parameters.
	std::vector<VecOption> _vectors; ///< Predefined vector parameters.
	std::vector<ColorOption> _colors; ///< Predefined color parameters.

	uint _frame = 0; ///< Current frame ID.
	double _currentTime = 0.0; ///< Current time.
	double _startTime = 0.0; ///< Time at which the shader began to play.
	bool _paused = false; ///< Should time/frame count flow.
	bool _showGUI = true; ///< Show the GUI parameters panel.
	bool _windowed = false; ///< Should the result be displayed in a subwindow.
	LayoutMode _layout = LayoutMode::HORIZONTAL; ///< The GUI panel layout.
	std::string _compilationLog; ///< Compilation log, will be displayed in a fixed panel if not empty.
};
