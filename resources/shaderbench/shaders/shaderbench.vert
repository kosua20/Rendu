
uniform mat4 iViewProjInv; ///< Inverse view proj matrix.

out INTERFACE {
	vec3 dir; ///< View world direction.
	vec2 uv; ///< Texture coordinates.
} Out ;

/** Render triangle on screen. Interpolate UV and view direction.*/
void main(){
	vec2 temp = 2.0 * vec2(gl_VertexID == 1, gl_VertexID == 2);
	Out.uv = temp;
	gl_Position.xy = 2.0 * temp - 1.0;
	gl_Position.zw = vec2(1.0);
	// Perform back projection to get a world space ray dir.
	Out.dir = vec3(iViewProjInv * gl_Position);
}
