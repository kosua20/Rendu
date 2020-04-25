#version 400

// Attributes
layout(location = 0) in vec3 v; ///< Position.

uniform mat4 mvp; ///< The transformation matrix.
uniform mat4 m; ///< Model transformation matrix.
uniform mat3 normalMatrix; ///< Normal transformation matrix.

out INTERFACE {
	vec3 pos; ///< World space position.
    vec3 nor; ///< Normal in world space.
} Out ;


/** Apply the MVP transformation to the input vertex. */
void main(){
	// We multiply the coordinates by the MVP matrix, and ouput the result.
	gl_Position = mvp * vec4(v, 1.0);
	// Store view space position.
	Out.pos = (m * vec4(v, 1.0)).xyz;
	// Compute the view space normal.
	Out.nor = normalize(normalMatrix * normalize(v));
}
