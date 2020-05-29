#version 400
in INTERFACE {
	vec2 uv;
} In ;

layout(binding=0) uniform sampler2D heightMap;
layout(binding=1) uniform sampler2D noiseTransition;
layout(binding=2) uniform sampler2DArray materials;
layout(binding=3) uniform sampler2DArray materialNormals;

layout (location = 0) out vec4 fragColor;

uniform vec3 lightDirection;
uniform bool debugCol;

vec3 mixNormals(vec3 n1, vec3 n2, float alpha){
	vec2 base = mix(n1.xy/n1.z, n2.xy/n2.z, alpha);
	return normalize(vec3(base, 1.0));
}

/** Shade the object, applying lighting. */
void main(){
	// Get clean normal and height.
	vec4 heightAndNor = textureLod(heightMap, In.uv, 0.0);
	vec3 n = normalize(heightAndNor.yzw);

	// Determine materials to use based on terrain height and orientation.
	float height = heightAndNor.x;
	// Disturb normal to break transitions between biomes.
	height += 1.5*textureLod(noiseTransition, In.uv*3.0, 0.0).x;
	float shoreToGround = smoothstep(0.0, 0.5, height);
	float groundToMountain = smoothstep(2.0, 2.5, height);
	float flatToSteep = pow(1.0-abs(n.y), 2.0);
	// Determine the four materials to blend (two regions, two orientations)
	int id0Flat, id0Steep, id1Flat, id1Steep;
	float regionTrans;
	if(shoreToGround < 1.0){
		regionTrans = shoreToGround;
		// 0 is sand/rock
		id0Flat = 2;
		id0Steep = 1;
		// 1 is grass/soil
		id1Flat = 3;
		id1Steep = 0;
	} else {
		regionTrans = groundToMountain;
		// 0 is grass/soil
		id0Flat = 3;
		id0Steep = 0;
		// 1 is snow/rock
		id1Flat = 4;
		id1Steep = 1;
	}

	// Read the colors, with a temporary tweak for snow.
	vec2 uv = In.uv*40.0;
	vec3 base0Flat = texture(materials, vec3(uv, id0Flat)).rgb;
	vec3 base0Steep = texture(materials, vec3(uv, id0Steep)).rgb;
	vec3 base1Flat = texture(materials, vec3(uv, id1Flat)).rgb;
	vec3 base1Steep = texture(materials, vec3(uv, id1Steep)).rgb;
	// Read the normals.
	vec3 base0FlatN =  texture(materialNormals, vec3(uv, id0Flat)).rgb * 2.0-1.0;
	vec3 base0SteepN = texture(materialNormals, vec3(uv, id0Steep)).rgb * 2.0-1.0;
	vec3 base1FlatN =  texture(materialNormals, vec3(uv, id1Flat)).rgb * 2.0-1.0;
	vec3 base1SteepN = texture(materialNormals, vec3(uv, id1Steep)).rgb * 2.0-1.0;
	// Blend color and normals.
	vec3 flatCol = mix(base0Flat, base1Flat, regionTrans);
	vec3 flatN = mixNormals(base0FlatN, base1FlatN, regionTrans);
	vec3 steepCol = mix(base0Steep, base1Steep, regionTrans);
	vec3 steepN = mix(base0SteepN, base1SteepN, regionTrans);
	vec3 baseCol = mix(flatCol, steepCol, flatToSteep);
	vec3 baseN = mix(flatN, steepN, flatToSteep);

	// Build an arbitrary normal frame and map local normal.
	vec3 tn = abs(n.y) < 0.01 ? vec3(0.0,1.0,0.0) : vec3(1.0, 0.0, 0.0);
	vec3 bn = normalize(cross(n, tn));
	tn = normalize(cross(bn, n));
	mat3 tbn = mat3(tn, bn, n);
	vec3 finalN = normalize(tbn * baseN);
	// Diffuse shading with extra tweak for snow.
	float light = max(0.0, dot(lightDirection, finalN))+(id1Flat == 4 ? 3.0 : 1.0) * 0.01;
	vec3 color = light * baseCol;

	if(debugCol){
		color = vec3(0.9,0.9,0.9);
	}

	fragColor = vec4(color,1.0);
}
