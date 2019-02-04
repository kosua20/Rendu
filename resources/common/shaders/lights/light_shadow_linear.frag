#version 330

in vec3 worldPos; ///< The world space position of the fragment.
uniform vec3 lightPositionWorld; ///< The world space position of the light.
uniform float lightFarPlane; ///< The light projection matrix far plane.
layout(location = 0) out vec2 fragColor; ///< World space depth and depth squared.

/** Compute the depth in world space, normalized by the far plane distance */
void main(){
	// We compute the distance in world space (or equivalently view space).
	// We normalize it by the far plane distance to obtain a [0,1] value.
	float dist = length(worldPos - lightPositionWorld) / lightFarPlane;
	fragColor = vec2(dist, dist*dist);
}
