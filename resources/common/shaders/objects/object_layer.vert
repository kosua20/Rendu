#version 330

// Attributes
layout(location = 0) in vec3 v; ///< Position.
layout(location = 2) in vec2 uv; ///< UVs.

uniform mat4 model; ///< The model to world transformation.

out GS_INTERFACE {
	vec4 pos; ///< World position.
	vec2 uv; ///< UV coordinates.
} Out;

/** Apply only the world transformation to the input vertex. */
void main(){
	Out.pos = model * vec4(v,1.0);
	Out.uv = uv;
}
