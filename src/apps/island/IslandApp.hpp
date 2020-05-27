#include "Terrain.hpp"

#include "Application.hpp"
#include "input/Input.hpp"
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

	std::unique_ptr<Framebuffer> _sceneBuffer; ///< Scene framebuffer.
	const Mesh * _skyMesh;
	const Program * _groundProgram;
	const Program * _skyProgram; ///< Atmospheric scattering shader.
	const Texture * _precomputedScattering; ///< Precomputed lookup table.

	const Program * _tonemap; ///< Tonemapping shader.

	bool _showWire = false;

	// Atmosphere options.
	glm::vec3 _lightDirection; ///< Sun light direction.

	std::unique_ptr<Terrain> _terrain;
	
	GPUQuery _prims = GPUQuery(GPUQuery::Type::PRIMITIVES_GENERATED);

	bool _showTerrain = false;
	bool _showOcean = true;
};
