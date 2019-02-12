#version 330

layout(location = 0) out vec4 fragColor; ///< Color.

in INTERFACE {
	vec3 n;
} In;

uniform vec3 baseColor;
uniform float ambientFactor = 0.2;

void main(){
	// Two directional ligths.
	const vec3 lightDir1 = normalize(vec3(1.0,1.0,1.0));
	const vec3 lightDir2 = normalize(vec3(-1.0,-0.8,1.0));
	vec3 nWorld = normalize(In.n);
	fragColor.rgb = baseColor * (max(0, dot(lightDir1, nWorld)) + 0.3*max(0, dot(lightDir2, nWorld)));
	fragColor.a = 1.0;
}
