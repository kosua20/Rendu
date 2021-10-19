#include "samplers.glsl"
#include "utils.glsl"

#define MATERIAL_EMISSIVE 0 ///< Emissive, non shaded material.
#define MATERIAL_STANDARD 1 ///< Basic PBR material (dielectric or metal).
#define MATERIAL_CLEARCOAT 2 ///< Clear coat & standard PBR material.

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
	material.id = decodeMaterial(albedoInfo.a);
	material.reflectance = albedoInfo.rgb;

	vec4 normalInfo = textureLod(sampler2D(gbuffer1, sClampNear), uv, 0.0);
	material.normal = decodeNormal(normalInfo.rg);

	vec4 effectInfo = textureLod(sampler2D(gbuffer2, sClampNear), uv, 0.0);
	material.roughness = max(0.045, effectInfo.r);
	material.ao = effectInfo.b;
	// Metalness is packed with something else.
	float parameter;
	decodeMetalnessAndParameter(effectInfo.g, material.metalness, parameter);

	// Decode clear coat if present.
	if(material.id == MATERIAL_CLEARCOAT){
		material.clearCoat = parameter;
		material.clearCoatRoughness = effectInfo.a;
	}

	return material;
}
