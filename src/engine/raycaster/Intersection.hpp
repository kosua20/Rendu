#pragma once
#include "resources/Mesh.hpp"
#include "Common.hpp"


/**
  \brief Represent a ray, shot from a given position in a specific direction.
  \ingroup Raycaster
 */
struct Ray {
	const glm::vec3 pos;	///< Ray position.
	const glm::vec3 dir;	///< Ray direction (normalized).
	const glm::vec3 invdir; ///< Ray reciprocal direction (normalized).

	/** Constructor.
	 \param origin the position the ray was shot from
	 \param direction the direction of the ray (will be normalized)
	 */
	Ray(const glm::vec3 & origin, const glm::vec3 & direction);
};


/**
 \brief Provide helpers for basic analytic intersections.
 \ingroup Raycaster
 */
class Intersection {

public:

	/** Check if a sphere of a given radius is intersected by a ray defined by an
		origin wrt to the sphere center and a normalized direction.
		\param rayOrigin the origin of the ray
		\param rayDir the direction of the ray (normalized)
		\param radius the radius of the sphere to intersect
		\param roots will contain the two roots of the associated polynomial, ordered.
		\return true if there is intersection.
		\warning The intersection can be in the negative direction along the ray. Check the sign of the roots to know.
	*/
	static bool sphere(const glm::vec3 & rayOrigin, const glm::vec3 & rayDir, float radius, glm::vec2 & roots);

	/** Test a ray and bounding box intersection.
	 \param ray the ray
	 \param box the bounding box
	 \param mini the minimum allowed distance along the ray
	 \param maxi the maximum allowed distance along the ray
	 \return a boolean denoting intersection
	 */
	static bool box(const Ray & ray, const BoundingBox & box, float mini, float maxi);

};
