
layout(location = 0) in INTERFACE {
	vec4 dir; ///< View world direction.
	vec2 uv; ///< Texture coordinates.
} In ;

layout(set = 1, binding = 0) uniform sampler2D previousFrame; ///< Previous frame.
layout(set = 1, binding = 1) uniform sampler2D sdfFont; ///< Font SDF texture.
layout(set = 1, binding = 2) uniform sampler2D gridMap; ///< Debug grid texture.
layout(set = 1, binding = 3) uniform sampler2D noiseMap; ///< RGBA uniform noise in [0,1], uncorrelated.
layout(set = 1, binding = 4) uniform sampler2D directionsMap; ///< Random 3D directions on the unit sphere.

layout(set = 0, binding = 0) uniform UniformBlock {
	float iTime; ///< Time since beginning of playback.
	float iTimeDelta; ///< Time since last frame.
	float iFrame; ///< Frame count since beginning of playback.
	vec3 iResolution; ///< Screen resolution.
	vec4 iMouse; ///< xy: mouse position if left button pressed, zw: left/right button clicked?
	mat4 iView; ///< View matrix.
	mat4 iProj; ///< Projection matrix.
	mat4 iViewProj; ///< View projection matrix.
	mat4 iViewInv; ///< Inverse view matrix.
	mat4 iProjInv; ///< Inverse projection matrix.
	mat4 iViewProjInv; ///< Inverse view projection matrix.
	mat4 iNormalMat; ///< Normal transformation matrix.
	vec3 iCamPos; ///< Camera position.
	vec3 iCamUp; ///< Camera up vector.
	vec3 iCamCenter; ///< Camera lookat position.
	float iCamFov; ///< Camera field of view.
	float gamma; ///< Gamma correction.
	float specExponent; ///< Specular exponent.
	float radius; ///< Sphere radius.
	float epsilon; ///< Raymarching tolerance.
	vec3 skyBottom; ///< Sky color bottom.
	vec3 skyLight; ///< Light color bottom.
	vec3 skyTop; ///< Sky color.
	vec3 lightColor; ///< Light color.
	vec3 sphereColor; ///< Sphere color.
	vec3 ground0; ///< Ground color 0.
	vec3 ground1; ///< Ground color 1.
	vec4 lightDirection; ///< Light direction.
	int stepCount; ///< Maximum step count.
	bool showPlane; ///< Show the moving plane.
};

layout(location = 0) out vec4 fragColor; ///< Output color.

/** Scene signed distance function.
\param pos the 3D world position
\return the distance to the surface and the material ID.
*/
vec2 map(vec3 pos){
	// Sphere 0.
	float dist0 = length(pos - vec3(0.0,0.1,-0.4)) - radius;
	float dist1 = abs(pos.y + 0.5);
	// Skip the plane if asked.
	return (dist0 < dist1 || !showPlane) ? vec2(dist0, 1.0) : vec2(dist1, 2.0);
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
	for(int i = 0; i < stepCount; ++i){
		// Current position.
		vec3 pos = orig + t * dir;
		// Query the distance to the closest surface in the scene.
		res = map(pos);
		// Move by this distance.
		t += res.x;
		// If the distance to the scene is small, we have reached the surface.
		if(res.x < epsilon){
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
	vec3 dir = normalize(In.dir.xyz);
	vec3 eye = iCamPos;
	// Light parameters.
	vec3 lightDir = normalize(lightDirection.xyz);

	// Background color: 
	// 4-directions gradient, centered on the light.
	float lightFacing = dot(dir,lightDir)*0.5+0.5;
	vec3 backgroundColor = mix(
		mix(skyBottom, skyLight, lightFacing), 
		mix(skyTop, lightColor, lightFacing), 
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
			baseColor = sphereColor;
			specular = pow(specular, specExponent);
		} else if(res.y < 2.5){
			vec2 movingUV = hit.xz + vec2(0.5*iTime, 0.0);
			float noise = texture(noiseMap, 0.005*movingUV).r;
			// Smooth transition.
			float intensity = smoothstep(0.45, 0.55, noise);
			// Mix two colors, diffuse material.
			baseColor = mix(ground0, ground1, intensity);
			specular = 0.0;
		}
		// Final appearance.
		vec3 objColor = 0.4 * baseColor + lightColor * (diffuse * baseColor + specular);
		// Apply a fading fog effect based on depth.
		color = mix(objColor, color, clamp(t-2.0,0.0,1.0));
	}
	
	/// Exposure tweak, output.
	fragColor = vec4(pow(color, vec3(gamma)), 1.0);
}
