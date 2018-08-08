#version 330

// Attributes
layout(location = 0) in vec3 v;

uniform mat4 model;

out GS_INTERFACE {
	vec4 pos;
} Out;

void main(){
	Out.pos = model * vec4(v,1.0);
}
