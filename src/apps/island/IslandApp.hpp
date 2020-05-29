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

/** \brief
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

	/** \copydoc CameraApp::clean */
	void clean() override;

private:

	void generateWaves();

	std::unique_ptr<Framebuffer> _sceneBuffer; ///< Scene framebuffer.
	std::unique_ptr<Framebuffer> _waterEffects; ///< Scene framebuffer.
	std::unique_ptr<Framebuffer> _waterEffectsBlur; ///< Scene framebuffer.
	const Mesh * _skyMesh;
	Mesh _oceanMesh;
	Mesh _farOceanMesh;
	Texture _transitionNoise;
	const Texture * _materials;
	const Texture * _materialNormals;
	const Program * _groundProgram;
	const Program * _oceanProgram;
	const Program * _farOceanProgram;
	const Program * _waterCopy;

	const Program * _skyProgram; ///< Atmospheric scattering shader.
	const Texture * _precomputedScattering; ///< Precomputed lookup table.

	const Program * _tonemap; ///< Tonemapping shader.

	bool _showWire = false;

	// Atmosphere options.
	glm::vec3 _lightDirection; ///< Sun light direction.

	// Ocean options.
	float _maxLevelX = 12;
	float _maxLevelY = 8;
	float _distanceScale = 1.0;

	std::unique_ptr<Terrain> _terrain;
	
	GPUQuery _prims = GPUQuery(GPUQuery::Type::PRIMITIVES_GENERATED);

	bool _showTerrain = true;
	bool _showOcean = true;

	struct GerstnerWave {
		glm::vec4 DiAngleActive = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
		glm::vec4 AQwp = glm::vec4(0.2f, 0.5f, 0.5f, 0.2f);
	};
	Buffer<GerstnerWave> _waves;
	
};
