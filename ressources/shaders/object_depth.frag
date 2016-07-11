#version 330


// Output: the fragment color
out vec3 fragColor;

void main(){
	
	fragColor = vec3(gl_FragCoord.z);
	
}
