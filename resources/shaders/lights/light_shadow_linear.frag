#version 330

in vec3 worldPos;
uniform vec3 lightPositionWorld;
uniform float lightFarPlane;
layout(location = 0) out vec2 fragColor;

void main(){
	// We compute the distance in world space (or equivalently view space).
	// We normalize it by the far plane distance to obtain a [0,1] value.
	float dist = length(worldPos - lightPositionWorld) / lightFarPlane;
	fragColor = vec2(dist, dist*dist);
}
