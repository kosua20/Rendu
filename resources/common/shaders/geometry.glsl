
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

/** Check if a box of a given extent is intersected by a ray defined by an
	origin and a normalized direction in the frame where the box is centered and axis-aligned.
	\param rayOrigin the origin of the ray
	\param rayDir the direction of the ray (normalized)
	\param extent the half size of the box in its local frame
	\param roots will contain the distances to the two intersections with the box
	\return true if there is intersection.
	\warning The intersections can be in the negative direction along the ray. Check the sign of the roots to know.
*/
bool intersectBox(vec3 rayOrigin, vec3 rayDir, vec3 extent, out vec2 roots){

	// We are already in local space.
	vec3 invDir = 1.0 / rayDir;
	vec3 minRatio = ( extent - rayOrigin) * invDir;
	vec3 maxRatio = (-extent - rayOrigin) * invDir;

	// Order the two sets of planes.
	vec3 minFinal = min(minRatio, maxRatio);
	vec3 maxFinal = max(minRatio, maxRatio);

	// Find the closest and furthest planes.
	roots.x = max(minFinal.x, max(minFinal.y, minFinal.z));
	roots.y = min(maxFinal.x, min(maxFinal.y, maxFinal.z));

	// No intersection if furthest < 0 or closest > furthest
	return max(roots.x, 0.0) <= roots.y;
}

/** Rotate a point around the vertical axis.
 \param p the point to trnasform
 \param csAngle precomputed cosine and sine of the rotation angle
 \return the transformed point
 */
vec3 rotateY(vec3 p, vec2 csAngle){
	mat3 rot = mat3(csAngle.x, 0.0, csAngle.y, 0.0, 1.0, 0.0,
					-csAngle.y, 0.0, csAngle.x);
	return rot * p;;
}
