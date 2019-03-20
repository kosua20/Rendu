#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ; ///< vec2 uv;

layout(binding = 0) uniform sampler2D normalMap; ///< Normal map.
layout(binding = 1) uniform sampler2D materialMap; ///< World pos and material index.
layout(binding = 2) uniform sampler2D ssaoMap; ///< SSAO result.
layout(binding = 3) uniform samplerCube envMap; ///< Environment map.

layout(location = 0) out vec4 fragColor; ///< Color.

void main(){
	
	vec4 posInd = texture(materialMap, In.uv);
	float matId = 255.0*posInd.w;
	vec3 n = normalize(2.0f * texture(normalMap, In.uv).rgb - 1.0f);
	float ao = texture(ssaoMap, In.uv).r;
	
	// Ground is white.
	vec3 baseColor = vec3(1.0);
	if(matId > 1.5){
		// Head is grey, body and items are red.
		baseColor = matId > 4.5  ? vec3(0.1) : (matId > 3.5 ? vec3(0.9) : vec3(0.9, 0.0, 0.0));
		// Computation for the reflection.
		// Intersect ray with sphere surrounding scene.
		vec3 pos = posInd.xyz;
		float nDotPos = dot(n, pos);
		const float radius = 10.0;
		float relativeRadius = dot(pos, pos) - radius*radius;
		float abscisse = -nDotPos + sqrt(nDotPos*nDotPos - relativeRadius);
		vec3 intersec = pos + abscisse * n;
		float atten = 1.0-pow(n.z, 16.0);
		// Composite reflection.
		baseColor *= mix(vec3(1.0), textureLod(envMap, intersec.xyz, 3.0).rgb, atten);
	}
	float light0 = max(0, 0.5*dot(n, normalize(vec3(0.0, 1.0, 1.0))) + 0.8);
	float adjustedAO = 0.7*ao + 0.3;
	baseColor *= light0 * adjustedAO;
	fragColor.rgb = baseColor;
	fragColor.a = 1.0f;
}
