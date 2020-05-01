#version 400

in INTERFACE {
	vec3 dir; ///< View world direction.
	vec2 uv; ///< Texture coordinates.
} In ;

uniform float iTime; ///< Time since beginning of playback.
uniform float iTimeDelta; ///< Time since last frame.
uniform float iFrame; ///< Frame count since beginning of playback.
uniform vec3 iResolution; ///< Screen resolution.
uniform vec4 iMouse; ///< xy: mouse position if left button pressed, zw: left/right button clicked?
uniform mat4 iView; ///< View matrix.
uniform mat4 iProj; ///< Projection matrix.
uniform mat4 iViewProj; ///< View projection matrix.
uniform mat4 iViewInv; ///< Inverse view matrix.
uniform mat4 iProjInv; ///< Inverse projection matrix.
uniform mat4 iViewProjInv; ///< Inverse view projection matrix.
uniform mat4 iNormalMat; ///< Normal transformation matrix.
uniform vec3 iCamPos; ///< Camera position.
uniform vec3 iCamUp; ///< Camera up vector.
uniform vec3 iCamCenter; ///< Camera lookat position.
uniform float iCamFov; ///< Camera field of view.

layout(binding = 0) uniform sampler2D previousFrame; ///< Previous frame.
layout(binding = 1) uniform sampler2D sdfFont; ///< Font SDF texture.
layout(binding = 2) uniform sampler2D gridMap; ///< Debug grid texture.
layout(binding = 3) uniform sampler2D noiseMap; ///< RGBA uniform noise in [0,1], uncorrelated.
layout(binding = 4) uniform sampler2D directionsMap; ///< Random 3D directions on the unit sphere.

layout(location = 0) out vec4 fragColor; ///< Output color.

uniform float float0 = 2.2; ///< Gamma correction.
uniform float float1 = 128.0; ///< Specular exponent.
uniform float float2 = 0.4; ///< Sphere radius.
uniform float float3 = 0.001; ///< Raymarching tolerance.

uniform vec4 col0 = vec4(0.0352416, 0.0901474, 0.159292, 0); ///< Sky color bottom.
uniform vec4 col1 = vec4(0, 0.254993, 0.654867, 0); ///< Light color bottom.
uniform vec4 col2 = vec4(0, 0.681416, 1, 0); ///< Sky color.
uniform vec4 col3 = vec4(0.99999, 1, 0.999992, 0); ///< Light color.
uniform vec4 col4 = vec4(0.8, 0.5, 0.2, 1.0); ///< Sphere color.

uniform vec4 vect0 = vec4(-1.8, 1.6, 3.8, 0.0); ///< Light direction.

uniform int int0 = 64; ///< Maximum step count.

/** Scene signed distance function.
\param pos the 3D world position
\return the distance to the surface and the material ID.
*/
vec2 map(vec3 pos){
	// Sphere 0.
	float dist0 = length(pos - vec3(0.0,0.0,0.0)) - float2;
	return vec2(dist0, 1.0);
}

/** Raymarch until hitting the scene surface or reaching the max number of steps.
\param orig the ray position
\param dir the ray direction
\param t will contain the distance along the ray to the intersection
\param res will contain final distance to the surface and material ID
\return true if there was an intersection
*/
bool raymarch(vec3 orig, vec3 dir, out float t, out vec2 res){
	// Reset.
	t = 0.0;
	res = vec2(0.0);
	// Step through the scene.
	for(int i = 0; i < int0; ++i){
		// Current position.
		vec3 pos = orig + t * dir;
		// Query the distance to the closest surface in the scene.
		res = map(pos);
		// Move by this distance.
		t += res.x;
		// If the distance to the scene is small, we have reached the surface.
		if(res.x < float3){
			return true;
		}
	}
	return false;
}

/** Compute the normal to the surface of the scene at a given world point.
\param p the point to evaluate the normal at
\return the normal
*/
vec3 normal(vec3 p){
	const vec2 epsilon = vec2(0.02, 0.0); //...bit agressive.
	float dP = map(p).x;
	// Forward differences scheme, cheaper.
	return normalize(vec3(
						  map(p + epsilon.xyy).x - dP,
						  map(p + epsilon.yxy).x - dP,
						  map(p + epsilon.yyx).x - dP
						  ));
}

/// Main render function.
void main(){
	vec3 dir = normalize(In.dir);
	vec3 eye = iCamPos;
	// Light parameters.
	vec3 lightDir = normalize(vect0.xyz);
	vec3 lightColor = col3.rgb;
	// Add some variation.
	float modulation = texture(noiseMap, vec2(0.5, iTime*0.01)).r * 0.2 + 0.9;
	lightColor *= modulation;

	// Background color: 
	// 4-directions gradient, centered on the light.
	float lightFacing = dot(dir,lightDir)*0.5+0.5;
	vec3 backgroundColor = mix(
		mix(col0.rgb, col1.rgb, lightFacing), 
		mix(col2.rgb, lightColor, lightFacing), 
		smoothstep(-0.1, 0.1, dir.y));
	vec3 color = backgroundColor;

	// Foreground:
	// Check if we intersect something along the ray.
	float t; vec2 res;
	bool didHit = raymarch(eye, dir, t, res);
	// If we hit a surface, compute its appearance.
	if(didHit){
		// Compute position and normal.
		vec3 hit = eye + t * dir;
		vec3 n = normal(hit);
		// Diffuse lighting.
		float diffuse = max(dot(n, lightDir), 0.0);
		// Specular lighting.
		float specular = max(dot(reflect(-lightDir, n), -dir), 0.0);
		// Adjust parameters based on surface hit.
		vec3 baseColor = vec3(0.0);
		if(res.y < 1.5){
			baseColor = col4.rgb;
			specular = pow(specular, float1);
		}
		// Final appearance.
		color = 0.4 * baseColor + lightColor * (diffuse * baseColor + specular);
	}
	
	/// Exposure tweak, output.
	fragColor = vec4(pow(color, vec3(float0)),1.0);
}
