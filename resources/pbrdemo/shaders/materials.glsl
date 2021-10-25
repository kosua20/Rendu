#include "samplers.glsl"
#include "utils.glsl"

#define MATERIAL_UNLIT 0 ///< Passthrough, non shaded material.
#define MATERIAL_STANDARD 1 ///< Basic PBR material (dielectric or metal).
#define MATERIAL_CLEARCOAT 2 ///< Clear coat & standard PBR material.
#define MATERIAL_EMISSIVE 3 ///< Emissive material with a specular dielectric layer.
#define MATERIAL_ANISOTROPIC 4 ///< Anisotropic GGX material.

/** Encode a material ID for storage in the G-buffer.
 \param material the identifier to encode
 \return the encoded value as a normalized float
 */
float encodeMaterial(uint material){
	return float(material)/255.0;
}

/** Decode a material ID from its G-buffer representation.
 \param raw the raw value stored in the Gbuffer
 \return the decoded material identifier
 */
uint decodeMaterial(float raw){
	return uint(raw * 255.0);
}

/** Encode metalness and another float parameter into a unique 8-bit normalized value,
 assigning 5 bits to metalness and 3 bits to the other parameter.
 \param metalness the metalness
 \param parameter the other parameter to pack
 \return a normalized float value storing both packed inputs
 */
float encodeMetalnessAndParameter(float metalness, float parameter){
	uint metal = uint(metalness * float((1 << 5) - 1));
	uint param = uint(parameter * float((1 << 3) - 1));
	uint packed = (param << 5) | metal;
	return float(packed) / 255.0;
}

/** Decode metalness and another float parameter from a unique 8-bit normalized value,
 using 5 bits for metalness and 3 bits ofr the other parameter.
 \param raw the packed input value
 \param metalness will contain the normalized unpacked metalness
 \param parameter will contain the normalized unpacked other parameter
 */
void decodeMetalnessAndParameter(float raw, out float metalness, out float parameter){
	uint packed = uint(raw * 255.0);
	uint metal = packed & ((1 << 5) - 1);
	uint param = (packed >> 5) & ((1 << 3) - 1);
	metalness = float(metal) / float((1 << 5) - 1);
	parameter = float(param) / float((1 << 3) - 1);
}

/** Store a surface point material parameters.
 */
struct Material {
	uint id; ///< Material ID.
	// Geometry
	vec3 normal; ///< Surface normal.
	// Standard
	vec3 reflectance; ///< Surface reflectance (base color).
	float metalness; ///< Dieletric/metal transition.
	float roughness; ///< Surface roughness.
	float ao; ///< Precomputed ambient occlusion.
	// Clear coat
	float clearCoat; ///< Clear coat intensity.
	float clearCoatRoughness; ///< Clear coat roughness.
	// Anisotropy
	float anisotropy; ///< Anisotropy intensity, from 1 (max in the tangent direction) to -1 (max in the bitangent direction), and 0 (none).
	vec3 tangent; ///< Tangent used for anisotropy, in view space.
	vec3 bitangent; ///< Bitangent used for anisotropy, in view space.

};

/** Fill a material with default parameters.
 \return the initialized material
 */
Material initMaterial(){
	Material params;
	params.id = MATERIAL_STANDARD;
	params.normal = vec3(0.0,1.0,0.0);
	params.reflectance = vec3(0.0);
	params.metalness = 0.0;
	params.roughness = 1.0;
	params.ao = 1.0;
	params.clearCoat = 0.0;
	params.clearCoatRoughness = 0.03;
	params.anisotropy = 0.0;
	params.tangent = vec3(1.0,0.0,0.0);
	params.bitangent = vec3(0.0,1.0,0.0);
	return params;
}

/** Populate material parameters from the G-buffer content.
 \param uv the location in the G-buffer
 \param gbuffer0 first texture of the G-buffer
 \param gbuffer1 second texture of the G-buffer
 \param gbuffer2 third texture of the G-buffer
 \return the decoded material parameters
 */
Material decodeMaterialFromGbuffer(vec2 uv, texture2D gbuffer0, texture2D gbuffer1, texture2D gbuffer2){
	// Initialize all parameters.
	Material material = initMaterial();

	vec4 albedoInfo = textureLod(sampler2D(gbuffer0, sClampNear),uv, 0.0);
	vec4 normalInfo = textureLod(sampler2D(gbuffer1, sClampNear), uv, 0.0);
	vec4 effectInfo = textureLod(sampler2D(gbuffer2, sClampNear), uv, 0.0);

	material.id = decodeMaterial(albedoInfo.a);
	material.normal = decodeNormal(normalInfo.rg);

	// Early exit for unlit.
	if(material.id == MATERIAL_UNLIT){
		return material;
	}

	material.reflectance = albedoInfo.rgb;

	material.roughness = max(0.045, effectInfo.r);
	material.ao = effectInfo.b;
	// Metalness is packed with something else.
	float parameter;
	decodeMetalnessAndParameter(effectInfo.g, material.metalness, parameter);

	// Emissive assumes a black diffuse, no AO and no metalness.
	if(material.id == MATERIAL_EMISSIVE){
		material.reflectance = vec3(0.0,0.0,0.0);
		material.metalness = 0.0;
		material.ao = 1.0;
		// Retrieve roughness from second texture.
		material.roughness = max(0.045, normalInfo.b);
		return material;
	}

	// Decode clear coat if present.
	if(material.id == MATERIAL_CLEARCOAT){
		material.clearCoat = parameter;
		material.clearCoatRoughness = effectInfo.a;
	}

	// Decode anisotropic parameters if present.
	// Based on the method described by S. Lagarde and E. Golubev in 
	// "The Road toward Unified Rendering with Unity’s High Definition Render Pipeline", 2015,
	// (http://advances.realtimerendering.com/s2018/index.htm#_9hypxp9ajqi)
	if(material.id == MATERIAL_ANISOTROPIC){
		// Signed anisotropy.
		material.anisotropy = (2.0 * effectInfo.a - 1.0);
		
		// Retrieve orientation.
		float orientationRaw = 2.0 * normalInfo.b - 1.0;
		// The sign denotes if we stored the sine or cosine.
		bool usedSine = (orientationRaw >= 0.0);
		// Denormalize value, assign to trigonometric parameters.
		float orientation = abs(orientationRaw) / sqrt(2.0);
		float otherOrientation = sqrt(1.0 - orientation * orientation);
		float sinFrame = usedSine ? orientation : otherOrientation;
		float cosFrame = usedSine ? otherOrientation : orientation;
		// Retrieve signs.
		uint signs = uint(3.0 * normalInfo.a);
		sinFrame *= ((signs & 1) != 0) ? -1.0 : 1.0;
		cosFrame *= ((signs & 2) != 0) ? -1.0 : 1.0;

		// Build default tangent frame from normal.
		vec3 defaultTangent, defaultBitangent;
		buildFrame(material.normal, defaultTangent, defaultBitangent);
		// Re-build current tangent frame from reference frame.
		material.tangent = normalize(cosFrame * defaultTangent + sinFrame * defaultBitangent);
		material.bitangent = normalize(cross(material.normal, material.tangent));
	}

	
	return material;
}
