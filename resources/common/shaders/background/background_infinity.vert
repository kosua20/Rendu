#version 400

// Attributes
layout(location = 0) in vec3 v; ///< Position.

out INTERFACE {
	vec2 uv; ///< UV coordinates.
} Out;

/** Output vertex as-is. We ensure the vertex will be set to the maximum depth by tweaking gl_Position.z.
 */
void main(){
	gl_Position = vec4(v, 1.0);
	// Ensure the quad is sent to the maximum depth.
	gl_Position.z = gl_Position.w; 
	Out.uv = 0.5*v.xy + 0.5;
	
}
