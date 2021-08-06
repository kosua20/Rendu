// Attributes
layout(location = 0) in vec3 v; ///< Position.

layout(set = 0, binding = 1) uniform UniformBlock {
	mat4 mvp; ///< The transformation matrix.
	mat4 m; ///< Model transformation matrix.
	mat4 normalMatrix; ///< Normal transformation matrix.
};

layout(location = 0) out INTERFACE {
	vec4 pos; ///< World space position.
    vec4 nor; ///< Normal in world space.
} Out;

/** Apply the MVP transformation to the input vertex. */
void main(){
	// We multiply the coordinates by the MVP matrix, and ouput the result.
	gl_Position = mvp * vec4(v, 1.0);
	// Store view space position.
	Out.pos.xyz = vec3(m * vec4(v, 1.0));
	Out.pos.w = 0.0;
	// Compute the view space normal.
	Out.nor.xyz = normalize(mat3(normalMatrix) * normalize(v));
	Out.nor.w = 0.0;
}
