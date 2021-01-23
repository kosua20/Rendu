#include "Terrain.hpp"

#include "Application.hpp"
#include "input/Input.hpp"
#include "processing/BoxBlur.hpp"
#include "resources/ResourcesManager.hpp"
#include "graphics/ScreenQuad.hpp"
#include "graphics/Framebuffer.hpp"
#include "graphics/GLUtilities.hpp"
#include "graphics/GPUObjects.hpp"
#include "generation/PerlinNoise.hpp"
#include "generation/Random.hpp"
#include "system/Window.hpp"
#include "system/System.hpp"
#include "system/Config.hpp"
#include "Common.hpp"

/** \brief Realistic rendering of a sandy island in the ocean.

 The terrain is rendered as a integer-shifted vertex grid as described by M. McGuire in his post "Fast Terrain Rendering with Continuous Detail on a Modern GPU", 2014 (http://casual-effects.blogspot.com/2014/04/fast-terrain-rendering-with-continuous.html). High-frequency sand shading is based on the "Sand Rendering in Journey" presentation, J. Edwards, GDC 2013 (https://www.youtube.com/watch?v=wt2yYnBRD3U).

 Ocean is tesselated on the fly based on the distance to the camera and displaced using Gerstner waves as described in "Effective Water Simulation from Physical Models", M. Finch, GPU Gems 2007 (https://developer.nvidia.com/gpugems/gpugems/part-i-natural-effects/chapter-1-effective-water-simulation-physical-models).
 Caustics, scattering and absorption effects are based on the Hitman "From Shore to Horizon: Creating a Practical Tessellation Based Solution" presentation, N. Longchamps, GDC 2017 (https://www.gdcvault.com/play/1024591/From-Shore-to-)
 Additional foam effects are inspired by the "Multi-resolution Ocean Rendering in Crest Ocean System" presentation, H. Bowles, Siggraph 2019 (http://advances.realtimerendering.com/s2019/index.htm).
 \ingroup Island
 */
class IslandApp final : public CameraApp {
public:

	/** Constructor
	 \param config rendering config
	 */
	IslandApp(RenderingConfig & config);

	/** \copydoc CameraApp::draw */
	void draw() override;

	/** \copydoc CameraApp::update */
	void update() override;

	/** \copydoc CameraApp::resize */
	void resize() override;

	/** Destructor. */
	~IslandApp() override;

private:

	/** Packed Gerstner wave parameters. */
	struct GerstnerWave {
		glm::vec4 DiAngleActive = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f); ///< 2D direction, angle and flag.
		glm::vec4 AQwp = glm::vec4(0.2f, 0.5f, 0.5f, 0.2f); ///< Gerstner wave parameters.
	};

	/** Generate waves with random parameters in predefined ranges. */
	void generateWaves();

	// Buffers.
	std::unique_ptr<Framebuffer> _sceneBuffer; ///< Scene framebuffer.
	std::unique_ptr<Framebuffer> _waterEffectsHalf; ///< Underwater terrain with caustics.
	std::unique_ptr<Framebuffer> _waterPos; ///< Underwater terrain world positions.
	std::unique_ptr<Framebuffer> _waterEffectsBlur; ///< Blurred underwater terrain.
	std::unique_ptr<Framebuffer> _environment; ///< Environment cubemap.
	BoxBlur _blur = BoxBlur(true, "Water"); ///< Underwater terrain blurring.

	// Geometry.
	std::unique_ptr<Terrain> _terrain; ///< Terrain generator and rendering data.
	const Mesh * _skyMesh; ///< Sky supporting mesh.
	Mesh _oceanMesh; ///< Ocean grid mesh.
	Mesh _farOceanMesh; ///< Far ocean supporting cylinder mesh.

	// Textures.
	const Texture * _caustics; ///< Caustics texture.
	const Texture * _waveNormals; ///< Small waves normal map.
	const Texture * _foam; ///< Foam texture.
	const Texture * _brdfLUT; ///< Linearized GGX BRDF look-up table.
	const Texture * _sandMapSteep; ///< Normal map for steep dunes.
	const Texture * _sandMapFlat; ///< Normal map for flat regions.
	const Texture * _precomputedScattering; ///< Precomputed lookup table for atmospheric scattering.
	const Texture * _absorbScatterOcean; ///< Precomputed lookup table for ocean absoprtion/scattering.
	Texture _surfaceNoise; ///< Sand surface normal noise.
	Texture _glitterNoise; ///< Specular sand noise.

	// Shaders.
	const Program * _groundProgram; ///< Terrain shader.
	const Program * _oceanProgram; ///< Ocean shader.
	const Program * _farOceanProgram; ///< Distant ocean simplified shader.
	const Program * _waterCopy; ///< Apply underwater terrain effects (caustics).
	const Program * _underwaterProgram; ///< Underwater rendering.
	const Program * _skyProgram; ///< Atmospheric scattering shader.
	const Program * _tonemap; ///< Tonemapping shader.

	// Atmosphere options.
	glm::vec3 _lightDirection; ///< Sun light direction.
	float _lightElevation = 6.0f; ///< Sun angular elevation.
	float _lightAzimuth = 43.0f; ///< Sun horizontal orientation.
	bool _shouldUpdateSky = true; ///< Should the environment map be updated at this frame.

	// Ocean options.
	Buffer<GerstnerWave> _waves; ///< Waves parameters.
	const int _gridOceanRes = 64; ///< Ocean grid resolution.
	float _maxLevelX = 1.0f; ///< Maximum level of detail.
	float _maxLevelY = 1.0f; ///< Maximum subdivision amount.
	float _distanceScale = 1.0f; ///< Extra distance scaling.

	// Debug.
	GPUQuery _primsGround = GPUQuery(GPUQuery::Type::PRIMITIVES_GENERATED); ///< Terrain generated primitives count.
	GPUQuery _primsOcean = GPUQuery(GPUQuery::Type::PRIMITIVES_GENERATED); ///< Ocean generated primitives count.
	bool _showTerrain = true; ///< Should the terrain be displayed.
	bool _showOcean = true; ///< Should the ocean be displayed.
	bool _showSky = true; ///< Should the sky be displayed.
	bool _stopTime = false; ///< Pause ocean animation.
	bool _showWire = false; ///< Show debug wireframe.
};
