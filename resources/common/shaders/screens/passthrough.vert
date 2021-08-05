layout(location = 0) in vec3 v;///< Position.

layout(location = 0) out INTERFACE {
	vec2 uv; ///< UV coordinates.
} Out ;

/**

*/
void main(){
	vec2 temp = 0.5 * v.xy + 0.5;
	Out.uv = temp;
	gl_Position.xy = v.xy;
	gl_Position.zw = vec2(1.0);
}
