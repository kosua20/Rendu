#version 330

// Attributes
layout(location = 0) in vec3 v;///< Position.

// Uniforms.

uniform vec2 position;
uniform vec2 scale;


void main(){
	gl_Position.xy = scale * v.xy + position;
	gl_Position.zw = vec2(0.0, 1.0);
}
