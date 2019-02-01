#version 330

// Attributes
layout(location = 0) in vec3 v; ///< Position.

uniform mat4 model; ///< The model to world transformation.

out GS_INTERFACE {
	vec4 pos;
} Out; ///< vec4 pos;

/** Apply only the world transformation to the input vertex. */
void main(){
	Out.pos = model * vec4(v,1.0);
}
