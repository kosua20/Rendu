#version 330

// Input: position in model space
in INTERFACE {
	vec3 position; 
} In ;

uniform samplerCube textureCubeMap;

// Output: the fragment color
out vec3 fragColor;

void main(){
	
	fragColor = texture(textureCubeMap,In.position).rgb;

}
