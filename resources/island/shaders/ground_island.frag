#version 400
in INTERFACE {
	vec2 uv;
} In ;

layout(binding=0) uniform sampler2D heightMap;

layout (location = 0) out vec4 fragColor;

uniform vec3 lightDirection;
uniform bool debugCol;

/** Shade the object, applying lighting. */
void main(){
	// Get clean normal and height.
	vec4 heightAndNor = textureLod(heightMap, In.uv, 0.0);
	vec3 n = normalize(heightAndNor.yzw);
	float height = heightAndNor.x;
	// CHeap diffuse shading with hand tweak terrain transitions.
	float light = max(0.0, dot(lightDirection, n))+0.01;
	float scaledElev = pow(abs(n.y), 4.0);
	vec3 baseColor = mix(vec3(0.5, 0.26, 0.12), vec3(0.3, 0.8, 0.1), smoothstep(0.02, 0.1, scaledElev));
	baseColor = mix(vec3(0.8, 0.8, 0.1), baseColor, smoothstep(0.0, 0.5, height));
	vec3 mountainColor = mix(vec3(0.1), vec3(1.0), smoothstep(0.02, 0.1, scaledElev));
	baseColor = mix(baseColor, mountainColor, smoothstep(1.0, 2.0, height));
	vec3 color = light*baseColor;

	if(debugCol){
		color = vec3(0.9,0.9,0.9);
	}
	fragColor = vec4(color,1.0);
}
