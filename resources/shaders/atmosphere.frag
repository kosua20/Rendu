#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ;

uniform mat4 camToWorld;
uniform mat4 clipToCam;
uniform vec3 viewPos;
uniform vec3 lightDirection;

const float groundRadius = 6371e3;
const float topRadius = 6471e3;

out vec3 fragColor;

#define M_PI 3.14159265358979323846


// Check if a sphere of a given radius is intersected by a ray defined by an origin wrt to the sphere center and a normalized direction.
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

vec3 computeRadiance(vec3 rayOrigin, vec3 rayDir, vec3 sunDir){
	// Check intersection with planet.
	vec2 interPlanet;
	bool didHit = intersects(rayOrigin, rayDir, groundRadius, interPlanet);
	// The intersection can be in front or behind the camera.
	// To only keep the forward intersection, a root should be positive.
	bool didHitForward = didHit && interPlanet.y >= 0.0;
	return vec3(float(didHitForward));
}

void main(){
	// Move to -1,1
	vec4 clipVertex = vec4(-1.0+2.0*In.uv, 0.0, 1.0);
	// Then to world space.
	vec3 viewRay = normalize((camToWorld*vec4((clipToCam*clipVertex).xyz, 0.0)).xyz);
	// We then move to the planet model space, where its center is in (0,0,0).
	vec3 planetSpaceViewPos = viewPos + vec3(0,6372e3,0);
	vec3 atmosphereColor = computeRadiance(planetSpaceViewPos, viewRay, lightDirection);
	fragColor = atmosphereColor;
}

