layout(location = 0) in vec3 v;///< Position.

layout(location = 0) out INTERFACE {
	vec3 pos; ///< Position.
} Out ;

layout(set = 0, binding = 1) uniform UniformBlock {
	vec3 up; ///< Face vertical vector.
	vec3 right; ///< Face horizontal vector.
	vec3 center; ///< Face center location.
};

/**
  Generate one triangle covering the whole screen,
   with according positions and cubemap positions based on vertices ID.
 \sa GLSL::Vert::Passthrough
*/
void main(){
	gl_Position.xy = v.xy;
	gl_Position.zw = vec2(1.0);
	Out.pos = center + gl_Position.x * right - gl_Position.y * up;
	
}
