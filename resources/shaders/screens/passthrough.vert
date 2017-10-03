#version 330

// Output: UV coordinates
out INTERFACE {
	vec2 uv;
} Out ;

/* Generate one triangle covering the whole screen,
   with according positions and UVs based on vertices ID.
 
 2: (-1,3),(0,2)
	 *
	 | \
	 |	 \
	 |	   \
	 |		 \
	 |		   \
	 *----------*  1: (3,-1), (2,0)
 0: (-1,-1), (0,0)
 
*/

void main(){
	
	Out.uv.x = gl_VertexID == 1 ? 2.0 : 0.0;
	Out.uv.y = gl_VertexID == 2 ? 2.0 : 0.0;

	gl_Position.xy = 2.0 * Out.uv - 1.0;
	gl_Position.zw = vec2(1.0);
	
}
