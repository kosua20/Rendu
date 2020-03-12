#version 330

#define MATERIAL_ID 1 ///< The material ID.

in INTERFACE {
	vec3 vn; ///< Normal in view space.
} In;

layout(binding = 0) uniform sampler2D texture0; ///< Albedo.
layout(binding = 2) uniform sampler2D texture2; ///< Effects map.

layout (location = 0) out vec4 fragColor; ///< Color.
layout (location = 1) out vec3 fragNormal; ///< View space normal.
layout (location = 2) out vec3 fragEffects; ///< Effects.

/** Transfer albedo and effects along with the material ID, and output the normal in view space. */
void main(){
	
	// View space normal.
	vec3 n = normalize(In.vn);
	
	// Store values.
	vec2 defaultUV = vec2(0.5);
	vec4 color = texture(texture0, defaultUV);
	if(color.a <= 0.01){
		discard;
	}
	fragColor.rgb = color.rgb;
	fragColor.a = float(MATERIAL_ID)/255.0;
	// Flip the normal for back facing faces.
	fragNormal.rgb = (gl_FrontFacing ? 1.0 : -1.0) * n * 0.5 + 0.5;
	fragEffects.rgb = texture(texture2, defaultUV).rgb;
	
}
