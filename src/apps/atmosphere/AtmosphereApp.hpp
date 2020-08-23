
#include "Application.hpp"
#include "scene/Sky.hpp"
#include "graphics/Framebuffer.hpp"
#include "system/Config.hpp"
#include "Common.hpp"

/** \brief Demo application for the atmospheric scattering shader.
 \ingroup AtmosphericScattering
 */
class AtmosphereApp final : public CameraApp {
public:
	
	/** Constructor
	 \param config rendering config
	 */
	AtmosphereApp(RenderingConfig & config);
	
	/** \copydoc CameraApp::draw */
	void draw() override;
	
	/** \copydoc CameraApp::update */
	void update() override;
	
	/** \copydoc CameraApp::resize */
	void resize() override;

	/**
	 Compute a scattering lookup table for real-time atmosphere rendering.
	 \param table the image to fill.
	 \param samples number of samples along a ray
	 */
	static void precomputeTable(Image & table, uint samples);
	
private:

	std::unique_ptr<Framebuffer> _atmosphereBuffer; ///< Scene framebuffer.
	const Program * _atmosphere; ///< Atmospheric scattering shader.
	const Program * _tonemap; ///< Tonemapping shader.
	const Texture * _precomputedScattering; ///< Precomputed lookup table.
	glm::vec3 _lightDirection; ///< Sun light direction.
	float _lightElevation = 10.0f; ///< Sun vertical angular elevation.
	float _lightAzimuth = 290.0f; ///< Sun horizontal orientation.
	float _altitude = 1.0f; ///< View altitude above the planet surface.
	Sky::AtmosphereParameters _atmoParams; ///< Atmosphere parameters.
};
