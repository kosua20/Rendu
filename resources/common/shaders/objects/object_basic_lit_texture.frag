#version 330

/// Interface block.
in INTERFACE {
	vec3 vn; ///< World space normal.
	vec2 uv; ///< Texture coordinates.
} In ; ///< vec3 vn; vec2 uv;

uniform vec3 lightDir = vec3(0.577); ///< Light direction.

layout(binding = 0) uniform sampler2D texture0; ///< Color texture.

layout(location = 0) out vec4 fragColor; ///< Color.

/** Render a textured mesh with three default lights on each side of the scene. */
void main(){
	vec4 baseColor = texture(texture0, In.uv);
	if(baseColor.a < 0.01){
		discard;
	}

	vec3 nn = normalize(In.vn);
	// Two basic lights.
	float light0 = 0.8*max(0.0, dot(nn, lightDir));
	float light1 = 0.6*max(0.0, dot(nn, -lightDir));
	float light2 = 0.3*max(0.0, dot(nn, lightDir*vec3(1.0,-1.0,1.0)));
	// Color the lights.
	fragColor.rgb = light0*vec3(0.8,0.9,1.0) + light1*vec3(1.0,0.9,0.8) + light2*vec3(0.8,1.0,0.8);
	fragColor.rgb *= baseColor.rgb;
	fragColor.a = 1.0;
	
}
