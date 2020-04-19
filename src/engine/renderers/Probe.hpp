#pragma once

#include "graphics/Framebuffer.hpp"
#include "renderers/Renderer.hpp"
#include "input/Camera.hpp"
#include "Common.hpp"

/**
 \brief Wrapper to apply a renderer to a cubemap, for instance to render a reflection probe.
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
	 \note If the renderer is using the output of the probe, be careful to note use it in the last rendering step.
	 */
	Probe(const glm::vec3 & position, std::shared_ptr<Renderer> renderer, uint size, uint mips, const glm::vec2 & clippingPlanes);

	/** Update the content of the cubemap. */
	void draw();

	void integrate(float clamp);

	/** Clean internal resources.
	 */
	void clean();

	/** \return the cubemap texture */
	const Texture * textureId() const {
		return _framebuffer->textureId();
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
	
private:

	std::unique_ptr<Framebuffer> _framebuffer; ///< The cubemap content.
	std::shared_ptr<Renderer> _renderer; ///< The renderer to use.

	std::array<Camera, 6> _cameras; ///< Camera for each face.
	std::array<glm::mat4, 6> _mvps; ///< MVP for each face.
	glm::vec3 _position; ///< The probe location.
};
