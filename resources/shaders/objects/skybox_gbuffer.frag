#version 330

#define MATERIAL_ID 0

// Input: position in model space
in INTERFACE {
	vec3 position; 
} In ;

uniform samplerCube texture0;

// Output: the fragment color
layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec3 fragNormal;
layout (location = 2) out vec3 fragEffects;


void main(){

	fragColor.rgb = textureLod(texture0, normalize(In.position), 0.0).rgb;
	fragColor.a = MATERIAL_ID;
	fragNormal = vec3(0.5);
	fragEffects = vec3(0.0);

}
