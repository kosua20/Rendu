#pragma once
#include "Common.hpp"
#include "raycaster/Raycaster.hpp"
#include "scene/Scene.hpp"


/**
 \brief Diffuse path tracer. Generates renderings of a scene by emitting rays from the user viewpoint and letting them bounce in the scene, forming paths. Lighting and materials contributions are accumulated along each path to compute the color of the associated sample.
 \ingroup PathtracerDemo
 */
class PathTracer {
public:
	
	/** Empty constructor. */
	PathTracer() = default;
	
	/** Constructor. Initializes the internal raycaster with the scene data.
	 \param scene the scene to path trace against
	 */
	PathTracer(const std::shared_ptr<Scene> & scene);
	
	/** Performs a rendering of the scene.
	 \param camera the viewpoint to use
	 \param samples the number of samples per-pixel
	 \param depth the maximum number of bounces for each path
	 \param render the image, will be filled with the (gamma-corrected) result
	 */
	void render(const Camera & camera, size_t samples, size_t depth, Image & render);
	
private:
	
	/** Helper to bilinearly sample a cubemap in a given direction.
	 \param images the six cubemap faces, in standard Rendu order (px, nx, py, ny, pz, nz)
	 \param dir the direction to sample
	 \return the sampled color.
	 */
	static glm::vec3 sampleCubemap(const std::vector<Image> & images, const glm::vec3 & dir);
	
	Raycaster _raycaster; ///< The internal raycaster.
	std::shared_ptr<Scene> _scene; ///< The scene.
	
};


