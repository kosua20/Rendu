#version 330

layout(location = 0) out vec4 fragColor; ///< Color.

uniform vec3 color;

void main(){
	
	fragColor = vec4(color, 1.0);
	
}
