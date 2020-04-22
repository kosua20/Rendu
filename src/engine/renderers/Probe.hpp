#pragma once

#include "resources/Buffer.hpp"
#include "resources/ResourcesManager.hpp"
#include "graphics/Framebuffer.hpp"
#include "renderers/Renderer.hpp"
#include "input/Camera.hpp"
#include "Common.hpp"

/**
 \brief A probe can be used to capture the appareance of a scene at a given location as a 360° cubemap.
 This is often used to render realistic real-time reflections and global illumination effects. It is
 recommended to split the rendering, radiance precomputation for GGX shading and irradiance SH decomposition
 over multiple frames as those steps are costly. Additional synchronization constraints are described
 for each function below.
 \ingroup Renderers
 */
class Probe {

public:

	/** Constructor
	 \param position the probe position in the scene
	 \param renderer the renderer to use to fill the cubemap
	 \param size the dimensions of the cubemap
	 \param mips the number of mip levels of the cubemap
	 \param clippingPlanes the near/far planes to use when rendering each face
	 \warning If the renderer is using the output of the probe, be careful to not use the probe content in the last rendering step.
	 */
	Probe(const glm::vec3 & position, std::shared_ptr<Renderer> renderer, uint size, uint mips, const glm::vec2 & clippingPlanes);

	/** Update the content of the cubemap. */
	void draw();

	/** Perform BRDF pre-integration of the probe radiance for increasing roughness and store them in the mip levels.
	 This also copies a downscaled version of the radiance for future SH computations.
	 \param clamp maximum intensity value, useful to avoid ringing artifacts
	 */
	void convolveRadiance(float clamp, size_t first, size_t count);

	/** Downscale radiance cubemap info for future irradiance estimation. */
	void prepareIrradiance();

	/** Estimate the SH representation of the cubemap irradiance. The estimation is done on the CPU,
	 and relies on downlaoding a (downscaled) copy of the cubemap content. For synchronization reasons,
	 it is recommended to only update irradiance every other frame, and to trigger the copy
	 (performed by prepareIrradiance) after the coeffs update. This will introduce a latency but
	 will avoid any stalls. */
	void estimateIrradiance();

	/** Clean internal resources.
	 */
	void clean();

	/** The cubemap containing the rendering. Its mip levels will store the preconvolved radiance
	 if convolveRadiance has been called
	 \return the cubemap texture
	 */
	Texture * textureId() const {
		return _framebuffer->textureId();
	}

	/** The cubemap irradiance SH representation, if estimateIrradiance has been called.
	 \return the irradiance SH coefficients
	 */
	const std::shared_ptr<Buffer<glm::vec4>> & shCoeffs() const {
		return _shCoeffs;
	}
	
	/** \return the probe position */
	const glm::vec3 & position() const {
		return _position;
	}

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


	/**
	\brief Decompose an existing cubemap irradiance onto the nine first elements of the spherical harmonic basis.
	\details Perform approximated convolution as described in Ramamoorthi, Ravi, and Pat Hanrahan.
	 "An efficient representation for irradiance environment maps.",
	 Proceedings of the 28th annual conference on Computer graphics and interactive techniques. ACM, 2001.
	\param cubemap the cubemap to extract SH coefficients from
	\param shCoeffs will contain the irradiance SH representation
	 */
	static void extractIrradianceSHCoeffs(const Texture & cubemap, std::vector<glm::vec3> & shCoeffs);
	
private:

	std::unique_ptr<Framebuffer> _framebuffer; ///< The cubemap content.
	std::shared_ptr<Renderer> _renderer; ///< The renderer to use.
	std::unique_ptr<Framebuffer> _copy; ///< Downscaled copy of the cubemap content.
	std::shared_ptr<Buffer<glm::vec4>> _shCoeffs; ///< SH representation of the cubemap irradiance.

	std::array<Camera, 6> _cameras; ///< Camera for each face.
	std::array<glm::mat4, 6> _mvps; ///< MVP for each (centered) face.
	glm::vec3 _position; ///< The probe location.
	const Program * _integration; ///< Radiance preconvolution shader.
	const Mesh * _cube; ///< Skybox cube.
};
