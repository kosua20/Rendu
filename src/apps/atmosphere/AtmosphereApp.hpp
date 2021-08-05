#pragma once
#include "Application.hpp"
#include "scene/Sky.hpp"
#include "graphics/Framebuffer.hpp"
#include "system/Config.hpp"
#include "Common.hpp"

/** \brief Demo application for the atmospheric scattering shader. Demonstrate real-time approximate atmospheric scattering simulation.
 * Based on Precomputed Atmospheric Scattering, E. Bruneton, F. Neyret, EGSR 2008
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

	~AtmosphereApp();

	/**
	 Compute a scattering lookup table for real-time atmosphere rendering.
	 \param params the atmosphere parameters
	 \param samples number of samples along a ray
	 \param table the image to fill.
	 */
	static void precomputeTable(const Sky::AtmosphereParameters & params, uint samples, Image & table);
	
private:

	/** Update the lookup table based on internal atmosphere parameters.*/
	void updateSky();

	std::unique_ptr<Framebuffer> _atmosphereBuffer; ///< Scene framebuffer.
	Program * _atmosphere; ///< Atmospheric scattering shader.
	Program * _tonemap; ///< Tonemapping shader.
	Texture _scattering; ///< Scattering lookup table.

	// Atmosphere parameters.
	Sky::AtmosphereParameters _atmoParams; ///< Atmosphere parameters.
	int _tableRes = 256; ///< Scattering table resolution.
	int _tableSamples = 64;	///< Scattering table sample count.
	
	// Real-time parameters.
	glm::vec3 _lightDirection;		///< Sun light direction.
	float _lightElevation = 10.0f;	///< Sun vertical angular elevation.
	float _lightAzimuth	  = 290.0f; ///< Sun horizontal orientation.
	float _altitude		  = 1.0f;	///< View altitude above the planet surface.
};
