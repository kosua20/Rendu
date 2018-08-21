#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ;

uniform mat4 camToWorld;
uniform mat4 clipToCam;
uniform vec3 viewPos;
uniform vec3 lightDirection;

out vec3 fragColor;

#define M_PI 3.14159265358979323846


void main(){
	// Move to -1,1
	vec4 clipVertex = vec4(-1.0+2.0*In.uv, 0.0, 1.0);
	// Then to world space.
	vec3 viewRay = normalize((camToWorld*vec4((clipToCam*clipVertex).xyz, 0.0)).xyz);
	
	fragColor = 0.5*viewRay+0.5;
}

