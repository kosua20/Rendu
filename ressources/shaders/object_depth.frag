#version 330


// Output: the fragment color
out float fragColor;

void main(){
	
	fragColor = gl_FragCoord.z;
	
}
