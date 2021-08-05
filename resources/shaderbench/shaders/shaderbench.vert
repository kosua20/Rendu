layout(location = 0) in vec3 v;///< Position.

layout(set = 0, binding = 1) uniform UniformBlock {
	mat4 iViewProjInv; ///< Inverse view proj matrix.
};

layout(location = 0) out INTERFACE {
	vec3 dir; ///< View world direction.
	vec2 uv; ///< Texture coordinates.
} Out ;

/** Render triangle on screen. Interpolate UV and view direction.*/
void main(){
	vec2 temp = 0.5 * v.xy + 0.5;
	Out.uv = temp;
	gl_Position.xy = v.xy;
	gl_Position.zw = vec2(1.0);
	gl_Position.y *= -1;
	// Perform back projection to get a world space ray dir.
	Out.dir = vec3(iViewProjInv * gl_Position);

}
