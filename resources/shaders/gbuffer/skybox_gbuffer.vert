#version 330

// Attributes
layout(location = 0) in vec3 v;

// Uniform
uniform mat4 mvp;

// Output: position in model space
out INTERFACE {
	vec3 position;
} Out ;


void main(){
	// We multiply the coordinates by the MVP matrix, and ouput the result.
	// To keep the skybox centered on the camera, we treat its vertices as directions (no translation)
	gl_Position = mvp * vec4(v, 0.0);
	// Ensure the skybox is sent to the maximum depth.
	gl_Position.z = gl_Position.w; 
	Out.position = v;
	
}
