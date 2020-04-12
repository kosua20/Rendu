#version 400

in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In;

uniform mat4 clipToWorld; ///< Clip-to-world space transformation matrix.
uniform vec3 viewPos; ///< The position in view space.
uniform vec3 lightDirection; ///< The light direction in world space.

const float groundRadius = 6371e3; ///< Radius of the planet.
const float topRadius = 6471e3; ///< Radius of the atmosphere.
const vec3 sunColor = vec3(1.474, 1.8504, 1.91198); ///< Sun direct color.

const float sunAngularRadius = 0.04675; ///< Sun angular radius.
const float sunAngularRadiusCos = 0.998; ///< Cosine of the sun angular radius.

layout (location = 0) out vec4 fragColor; ///< Color.

#define M_PI 3.14159265358979323846


/** Check if a sphere of a given radius is intersected by a ray defined by an 
	origin wrt to the sphere center and a normalized direction.
	\param rayOrigin the origin of the ray
	\param rayDir the direction of the ray (normalized)
	\param radius the radius of the sphere to intersect
	\param roots will contain the two roots of the associated polynomial, ordered.
	\return true if there is intersection.
	\warning The intersection can be in the negative direction along the ray. Check the sign of the roots to know.
*/
bool intersects(vec3 rayOrigin, vec3 rayDir, float radius, out vec2 roots){
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

/** Cheap sky color estimation.
	\param rayOrigin the ray origin
	\param rayDir the ray direction
	\param sunDir the light direction
	\return the estimated radiance
*/
vec3 computeEstimate(vec3 rayOrigin, vec3 rayDir, vec3 sunDir){
	// Check intersection with atmosphere.
	vec2 interTop, interGround;
	bool didHitTop = intersects(rayOrigin, rayDir, topRadius, interTop);
	// If no intersection with the atmosphere, it's the dark void of space.
	if(!didHitTop){
		return vec3(0.0);
	}
	// Now intersect with the planet.
	bool didHitGround = intersects(rayOrigin, rayDir, groundRadius, interGround);
	
	// The sun itself if we're looking at it.
	vec3 sunRadiance = vec3(0.0);
	bool didHitGroundForward = didHitGround && interGround.y > 0;
	if(!didHitGroundForward && dot(rayDir, sunDir) > sunAngularRadiusCos){
		sunRadiance = sunColor / (M_PI * sunAngularRadius * sunAngularRadius);
	}

	float sunHeight = clamp(sunDir.y, 0.0, 1.0);
	float rayHeight = clamp(rayDir.y, 0.0, 1.0);
	// Approximate sky color with gradients wrt the sun height (time of day) and ray height (region of the hemisphere).
	vec3 bottomSkyColor = mix(vec3(0.831, 0.439, 0.141), vec3(0.615, 0.788, 0.956), sunHeight);
	vec3 topSkyColor = mix(vec3(0.333, 0.333, 0.317), vec3(0.439, 0.639, 0.839), sunHeight);
	vec3 groundColor = mix(vec3(0.018, 0.005, 0.0), vec3(0.011, 0.01, 0.008), sunHeight);
	return sunRadiance + (didHitGroundForward ? groundColor : mix(bottomSkyColor, topSkyColor, rayHeight));
}

/** Simulate sky color based on an atmospheric scattering approximate model. */
void main(){
	// Move to -1,1
	vec4 clipVertex = vec4(-1.0+2.0*In.uv, 0.0, 1.0);
	// Then to world space.
	vec3 viewRay = normalize((clipToWorld * clipVertex).xyz);
	// We then move to the planet model space, where its center is in (0,0,0).
	vec3 planetSpaceViewPos = viewPos + vec3(0,6371e3,0) + vec3(0.0,1.0,0.0);
	vec3 atmosphereColor = computeEstimate(planetSpaceViewPos, viewRay, lightDirection);
	fragColor.rgb = atmosphereColor;
	fragColor.a = 1.0;
}

