#version 330

layout(location = 0) out vec2 fragColor; ///< Depth and depth squared.

/** Output the final depth of the fragment and its square, for variance shadow mapping. */
void main(){
	
	fragColor = vec2(gl_FragCoord.z,gl_FragCoord.z*gl_FragCoord.z);
	
}
