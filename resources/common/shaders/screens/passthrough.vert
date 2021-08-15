layout(location = 0) in vec3 v;///< Position.

layout(location = 0) out INTERFACE {
	vec2 uv; ///< UV coordinates.
} Out ;

/**
 Passthrough screen covering triangle.
*/
void main(){
	Out.uv = 0.5 * v.xy + 0.5;
	gl_Position.xy = v.xy;
	gl_Position.zw = vec2(1.0);
}
