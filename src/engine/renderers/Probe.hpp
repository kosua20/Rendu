#pragma once

#include "resources/Buffer.hpp"
#include "resources/ResourcesManager.hpp"
#include "graphics/Framebuffer.hpp"
#include "renderers/Renderer.hpp"
#include "input/Camera.hpp"
#include "Common.hpp"

class LightProbe;

/**
 \brief A probe can be used to capture the appareance of a scene at a given location as a 360° cubemap.
 \details This is often used to render realistic real-time reflections and global illumination effects. It is
 recommended to split the rendering, radiance precomputation for GGX shading and irradiance SH decomposition
 over multiple frames as those steps are costly.
 \ingroup Renderers
 */
class Probe {

public:

	/** Constructor
	 \param probe the probe in the scene
	 \param renderer the renderer to use to fill the cubemap
	 \param size the dimensions of the cubemap
	 \param mips the number of mip levels of the cubemap
	 \param clippingPlanes the near/far planes to use when rendering each face
	 \warning If the renderer is using the output of the probe, be careful to not use the probe content in the last rendering step.
	 */
	Probe(LightProbe & probe, std::shared_ptr<Renderer> renderer, uint size, uint mips, const glm::vec2 & clippingPlanes);

	/** Update the content of the probe and the corresponding radiance and irradiance.
	 \param budget the number of internal update steps to perform
	 Each internal step (drawing a part of the environment, generating the convolved radiance, integrating the irradiance) has a given budget. Depending on the allocated budget, the probe will entirely update more or less fast. */
	void update(uint budget);

	/** \return the total number of steps to completely update the probe data */
	uint totalBudget() const;

	/** Copy assignment operator (disabled).
	 \return a reference to the object assigned to
	 */
	Probe & operator=(const Probe &) = delete;
	
	/** Copy constructor (disabled). */
	Probe(const Probe &) = delete;
	
	/** Move assignment operator (disabled).
	 \return a reference to the object assigned to
	 */
	Probe & operator=(Probe &&) = delete;
	
	/** Move constructor (disabled). */
	Probe(Probe &&) = delete;

	/** Destructor */
	~Probe() = default;

	/**
	\brief Decompose an existing cubemap irradiance onto the nine first elements of the spherical harmonic basis.
	\details Perform approximated convolution as described in Ramamoorthi, Ravi, and Pat Hanrahan.
	 "An efficient representation for irradiance environment maps.",
	 Proceedings of the 28th annual conference on Computer graphics and interactive techniques. ACM, 2001.
	\param cubemap the cubemap to extract SH coefficients from
	\param clamp maximum intensity value, useful to avoid temporal instabilities
	\param shCoeffs will contain the irradiance SH representation
	 */
	static void extractIrradianceSHCoeffs(const Texture & cubemap, float clamp, std::vector<glm::vec3> & shCoeffs);
	
private:

	/** Perform BRDF pre-integration of the probe radiance for increasing roughness and store them in the mip levels.
	 \param clamp maximum intensity value, useful to avoid ringing artifacts
	 \param layer first layer to process (in 1, mip count - 1)
	 */
	void convolveRadiance(float clamp, uint layer);

	/** Estimate the SH representation of the cubemap irradiance. The estimation is done on the GPU.
	 \param clamp maximum intensity value, useful to avoid temporal instabilities
	 */
	void estimateIrradiance(float clamp);

	std::unique_ptr<Framebuffer> _framebuffer; ///< The cubemap content.
	std::shared_ptr<Renderer> _renderer; ///< The renderer to use.
	std::unique_ptr<Framebuffer> _copy; ///< Downscaled copy of the cubemap content.
	std::shared_ptr<Buffer> _shCoeffs; ///< SH representation of the cubemap irradiance.

	std::array<Camera, 6> _cameras; ///< Camera for each face.
	glm::vec3 _position; ///< The probe location.
	Program* _radianceCompute; ///< Radiance preconvolution shader.
	Program* _irradianceCompute; ///< Irradiance SH projection shader.

	/// \brief Probe update state machine.
	enum class ProbeState {
		DRAW_FACES, ///< Drawing a cubemap face with the environment.
		CONVOLVE_RADIANCE, ///< Convolving the cubemap to generate radiance for a given roughness level.
		GENERATE_IRRADIANCE ///< Integrate irradiance in a compute shader.
	};
	// Initial state machine status.
	ProbeState _currentState = ProbeState::DRAW_FACES; ///< Current update state.
	uint _substepDraw = 0; ///< If drawing, current face.
	uint _substepRadiance = 1; ///< If convolving radiance, current level.
};
