#include "raycaster/Intersection.hpp"

Ray::Ray(const glm::vec3 & origin, const glm::vec3 & direction) :
	pos(origin), dir(glm::normalize(direction)), invdir(1.0f / glm::normalize(direction)) {
}

bool Intersection::sphere(const glm::vec3 & rayOrigin, const glm::vec3 & rayDir, float radius, glm::vec2 & roots){
	const float a = glm::dot(rayDir,rayDir);
	const float b = glm::dot(rayOrigin, rayDir);
	const float c = glm::dot(rayOrigin, rayOrigin) - radius*radius;
	const float delta = b*b - a*c;
	// No intersection if the polynome has no real roots.
	if(delta < 0.0f){
		return false;
	}
	// If it intersects, return the two roots.
	const float dsqrt = std::sqrt(delta);
	roots = (glm::vec2(-b-dsqrt,-b+dsqrt))/a;
	return true;
}

bool Intersection::box(const Ray & ray, const BoundingBox & box, float mini, float maxi) {
	const glm::vec3 minRatio = (box.minis - ray.pos) * ray.invdir;
	const glm::vec3 maxRatio = (box.maxis - ray.pos) * ray.invdir;
	const glm::vec3 minFinal = glm::min(minRatio, maxRatio);
	const glm::vec3 maxFinal = glm::max(minRatio, maxRatio);

	const float closest  = std::max(minFinal[0], std::max(minFinal[1], minFinal[2]));
	const float furthest = std::min(maxFinal[0], std::min(maxFinal[1], maxFinal[2]));

	return std::max(closest, mini) <= std::min(furthest, maxi);
}
