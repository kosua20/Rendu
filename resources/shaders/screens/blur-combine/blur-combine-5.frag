#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ;

// Uniforms: the textures.
layout(binding = 0) uniform sampler2D texture0;
layout(binding = 1) uniform sampler2D texture1;
layout(binding = 2) uniform sampler2D texture2;
layout(binding = 3) uniform sampler2D texture3;
layout(binding = 4) uniform sampler2D texture4;

// Output: the fragment color
layout(location = 0) out vec3 fragColor;


void main(){
	vec3 col = texture(texture0, In.uv).rgb;
	col += texture(texture1, In.uv).rgb;
	col += texture(texture2, In.uv).rgb;
	col += texture(texture3, In.uv).rgb;
	col += texture(texture4, In.uv).rgb;
	fragColor = col / 5.0;
}
