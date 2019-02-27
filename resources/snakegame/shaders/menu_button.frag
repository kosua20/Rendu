#version 330

layout(location = 0) out vec4 fragColor; ///< Color.

uniform vec4 color;

void main(){
	
	fragColor = color;
	
}
