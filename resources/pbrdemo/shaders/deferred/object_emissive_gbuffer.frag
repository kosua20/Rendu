
#define MATERIAL_ID 0 ///< The material ID.

layout(location = 0) in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In ;

layout(set = 1, binding = 0) uniform sampler2D texture0; ///< Emissive.

layout (location = 0) out vec4 fragColor; ///< Color.
layout (location = 1) out vec3 fragNormal; ///< View space normal.
layout (location = 2) out vec3 fragEffects; ///< Effects.

/** Transfer emissive. */
void main(){
	
	vec4 color = texture(texture0, In.uv);
	if(color.a <= 0.01){
		discard;
	}
	
	// Store values.
	fragColor.rgb = color.rgb;
	fragColor.a = float(MATERIAL_ID)/255.0;
	fragNormal = vec3(0.5);
	fragEffects = vec3(0.0);
}
