layout(location = 0) in vec3 v;///< Position.

layout(set = 0, binding = 1) uniform UniformBlock {
	bool flip; ///< Flip vertically.
};

layout(location = 0) out INTERFACE {
	vec2 uv; ///< UV coordinates.
} Out ;

/**

*/
void main(){
	vec2 temp = 0.5 * v.xy + 0.5;
	Out.uv = flip ? vec2(temp.x, 1.0-temp.y) : temp;
	gl_Position.xy = v.xy;
	gl_Position.zw = vec2(1.0);
	gl_Position.y *= -1;
}
