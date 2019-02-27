#version 330

// Attributes
layout(location = 0) in vec3 v;///< Position.

// Uniforms.

uniform vec2 position;
uniform vec2 scale;
uniform float depth;

// Output: UV coordinates
out INTERFACE {
	vec2 uv;
} Out ; ///< vec2 uv;

void main(){
	gl_Position.xy = scale * v.xy + position;
	gl_Position.zw = vec2(depth, 1.0);
	Out.uv = 0.5 * v.xy + 0.5;
}
