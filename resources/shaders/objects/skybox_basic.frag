#version 330

// Input: position in model space
in INTERFACE {
	vec3 pos;
} In ;

uniform samplerCube texture0;

// Output: the fragment color
out vec4 fragColor;


void main(){
	fragColor = texture(texture0, normalize(In.pos));
}
