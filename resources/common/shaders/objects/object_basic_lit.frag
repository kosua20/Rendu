#version 330

/// World space normal.
in INTERFACE {
	vec3 vn;
} In ; ///< vec3 vn;

uniform vec3 lightDir = vec3(0.577); ///< Light direction.

layout(location = 0) out vec4 fragColor; ///< Color.

/** Render a mesh with two default lights on each side of the scene. */
void main(){
	vec3 nn = normalize(In.vn);
	// Two basic lights.
	float light0 = 0.8*max(0.0, dot(nn, lightDir));
	float light1 = 0.2*max(0.0, dot(nn, -lightDir));
	// Color the lights.
	fragColor.rgb = light0*vec3(0.6,0.8,1.0) + light1*vec3(1.0,0.7,0.6);
	fragColor.a = 1.0;
}
