#version 330

uniform vec3 lightColor; ///< The color of the light.

// Output: the fragment color
layout (location = 0) out vec4 fragColor; ///< Color.
layout (location = 1) out vec3 fragNormal; ///< Normal (view space).
layout (location = 2) out vec3 fragEffects; ///< Effects (roughness, etc.).

/** Simply output the light color, and identify as background (no additional processing) */
void main(){
	
	// Store values.
	fragColor.rgb = lightColor;
	fragColor.a = 0.0; // same ID as the background.
	fragNormal.rgb = vec3(0.5);
	fragEffects.rgb = vec3(0.0);
	
}
