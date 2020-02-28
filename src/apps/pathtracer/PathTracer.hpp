#pragma once
#include "raycaster/Raycaster.hpp"
#include "scene/Scene.hpp"
#include "Common.hpp"

/**
 \brief Unidirectional path tracer. Generates renderings of a scene by emitting rays from the user viewpoint and letting them bounce in the scene, forming paths. Lighting and materials contributions are accumulated along each path to compute the color of the associated sample.
 \ingroup PathtracerDemo
 */
class PathTracer {
public:
	/** Empty constructor. */
	PathTracer() = default;

	/** Constructor. Initializes the internal raycaster with the scene data.
	 \param scene the scene to path trace against
	 */
	explicit PathTracer(const std::shared_ptr<Scene> & scene);

	/** Performs a rendering of the scene.
	 \param camera the viewpoint to use
	 \param samples the number of samples per-pixel
	 \param depth the maximum number of bounces for each path
	 \param render the image, will be filled with the (gamma-corrected) result
	 */
	void render(const Camera & camera, size_t samples, size_t depth, Image & render);

	/** \return the internal raycaster. */
	const Raycaster & raycaster() const { return _raycaster; }

private:

	/** Compute the dimensions of a grid that contains a given number of samples.
	 \param samples the number of samples to place on a regular grid
	 \return the number of samples on each axis
	 */
	static glm::ivec2 getSampleGrid(size_t samples);

	/** Get the local location of a sample in a pixel.
	 \param sid the sample ID for the pixel
	 \param cellCount the number of samples on each grid axis
	 \param cellSize the spacing between two samples on an axis
	 \note The sample will be randomly jittered.
	 */
	static glm::vec2 getSamplePosition(size_t sid, const glm::ivec2 & cellCount, const glm::vec2 & cellSize);

	/** Build the local frame at an intersection on an object surface.
	 \param obj the intersected object
	 \param hit the intersection record
	 \param rayDir the direction of the ray that intersected
	 \param uv the local texture coordinates (if valid)
	 \return the local tangent space frame.
	 \*/
	static glm::mat3 buildLocalFrame(const Object & obj, const Raycaster::Hit & hit, const glm::vec3 & rayDir, const glm::vec2 & uv);

	/** Check visibility from a point along a ray in the scene, taking into account object opacity masks.
	 \param startPos the point to test visibility for
	 \param rayDir the ray direction to follow
	 \param maxDist the maximum distance to travel along the ray
	 \return true if the point has clear visibility along the ray
	 */
	bool checkVisibility(const glm::vec3 & startPos, const glm::vec3 & rayDir, float maxDist) const;

	/** Evalutation the contribution from the scene background.
	 \param rayDir the direction of the ray that intersected
	 \param ndfPos the current pixel in the final image
	 \param directHit was it a direct hit or a hit after bounces
	 \return the background contribution
	 */
	glm::vec3 evalBackground(const glm::vec3 & rayDir, const glm::vec2 & ndcPos, bool directHit) const;

	Raycaster _raycaster;		   ///< The internal raycaster.
	std::shared_ptr<Scene> _scene; ///< The scene.
};
