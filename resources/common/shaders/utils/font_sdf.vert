#version 330

// Attributes
layout(location = 0) in vec3 v;///< Position.
layout(location = 1) in vec2 uv;///< Position.

uniform float ratio = 1.0f;
uniform vec2 position;

// Output: UV coordinates
out INTERFACE {
	vec2 uv;
} Out ; ///< vec2 uv;



void main(){
	gl_Position.xy = position + v.xy * vec2(ratio, 1.0);
	gl_Position.zw = vec2(1.0);
	Out.uv = uv ;
}
