#version 330

// Attributes
layout(location = 0) in vec3 v; ///< Position.

// Uniform
uniform mat4 mvp; ///< MVP transformation matrix.

// Output: position in model space
out INTERFACE {
	vec2 uv;
} Out ; ///< vec3 position;

/** Apply the transformation to the input vertex, treating it as a vector to ignore the translation part and keep it centered.
 We also ensure the vertex will be set to the maximum depth by tweaking gl_Position.z.
 */
void main(){
	gl_Position = vec4(v, 1.0);
	// Ensure the quad is sent to the maximum depth.
	gl_Position.z = gl_Position.w; 
	Out.uv = 0.5*v.xy + 0.5;
	
}
