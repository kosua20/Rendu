
/** Check if a sphere of a given radius is intersected by a ray defined by an
	origin wrt to the sphere center and a normalized direction.
	\param rayOrigin the origin of the ray
	\param rayDir the direction of the ray (normalized)
	\param radius the radius of the sphere to intersect
	\param roots will contain the two roots of the associated polynomial, ordered.
	\return true if there is intersection.
	\warning The intersection can be in the negative direction along the ray. Check the sign of the roots to know.
*/
bool intersectSphere(vec3 rayOrigin, vec3 rayDir, float radius, out vec2 roots){
	float a = dot(rayDir,rayDir);
	float b = dot(rayOrigin, rayDir);
	float c = dot(rayOrigin, rayOrigin) - radius*radius;
	float delta = b*b - a*c;
	// No intersection if the polynome has no real roots.
	if(delta < 0.0){
		return false;
	}
	// If it intersects, return the two roots.
	float dsqrt = sqrt(delta);
	roots = (-b+vec2(-dsqrt,dsqrt))/a;
	return true;
}

