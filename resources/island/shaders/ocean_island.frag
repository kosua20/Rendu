#version 400

in INTERFACE {
	vec3 pos;
} In ;

uniform vec3 camPos;
uniform vec3 lightDirection;
uniform bool debugCol;
uniform float time;
uniform float texelSize;
uniform float invMapSize;
uniform bool raycast;

layout(binding = 0) uniform sampler2D heightMap;


layout (location = 0) out vec4 fragColor;

const vec3 sunColor = vec3(1.474, 1.8504, 1.91198);

/** Shade the object, applying lighting. */
void main(){

	vec3 worldPos;
	float viewDist;
	vec3 vdir;
		worldPos = In.pos;
		vec3 dView = worldPos - camPos;
		viewDist = length(dView);
		vdir = dView/max(viewDist, 0.001);

	// Apply a basic Phong lobe for now.
	vec3 nn = vec3(0.0);
	nn.y += 1.0;
	vec3 n = normalize(nn);

	float diffuse = max(0.0, dot(lightDirection, n));
	vec3 ldir = reflect(vdir, n);
	float specular = pow(max(0.0, dot(ldir, lightDirection)), 1024.0);
	vec3 baseColor = vec3(0.02, 0.1, 0.2);

	vec3 color = sunColor * (specular + (diffuse + 0.01) * baseColor);
	if(debugCol){
		color = vec3(0.9);
	}
	fragColor = vec4(color,1.0);
}
