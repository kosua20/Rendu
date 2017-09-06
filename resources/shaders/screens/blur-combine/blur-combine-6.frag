#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ;

// Uniforms: the textures.
uniform sampler2D texture0;
uniform sampler2D texture1;
uniform sampler2D texture2;
uniform sampler2D texture3;
uniform sampler2D texture4;
uniform sampler2D texture5;

// Output: the fragment color
out vec3 fragColor;


void main(){
	vec3 col = texture(texture0, In.uv).rgb;
	col += texture(texture1, In.uv).rgb;
	col += texture(texture2, In.uv).rgb;
	col += texture(texture3, In.uv).rgb;
	col += texture(texture4, In.uv).rgb;
	col += texture(texture5, In.uv).rgb;
	fragColor = col / 6.0;
}