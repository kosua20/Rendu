layout(location = 0) in vec3 v;///< Position.

layout(binding = 0) uniform UniformBlock {
	bool flip; ///< Flip vertically.
};

layout(location = 0) out INTERFACE {
	vec2 uv; ///< UV coordinates.
} Out ;

/**
  Generate one triangle covering the whole screen,
   with according positions and UVs based on vertices ID.
 \verbatim
 2: (-1,3),(0,2)
	 *
	 | \
	 |	 \
	 |	   \
	 |		 \
	 |		   \
	 *-----------*  1: (3,-1), (2,0)
 0: (-1,-1), (0,0)
 \endverbatim
*/
void main(){
	vec2 temp = 0.5 * v.xy + 0.5;
	Out.uv = flip ? vec2(temp.x, 1.0-temp.y) : temp;
	gl_Position.xy = v.xy;
	gl_Position.zw = vec2(1.0);
}
