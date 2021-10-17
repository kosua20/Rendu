#include "samplers.glsl"
#include "utils.glsl"

#define MATERIAL_EMISSIVE 0 ///< Emissive, non shaded material.
#define MATERIAL_STANDARD 1 ///< Basic PBR material (dielectric or metal).

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
	Material params = initMaterial();

	vec4 albedoInfo = textureLod(sampler2D(gbuffer0, sClampNear),uv, 0.0);
	params.id = decodeMaterial(albedoInfo.a);
	params.reflectance = albedoInfo.rgb;

	vec4 normalInfo = textureLod(sampler2D(gbuffer1, sClampNear), uv, 0.0);
	params.normal = decodeNormal(normalInfo.rg);

	vec4 effectInfo = textureLod(sampler2D(gbuffer2, sClampNear), uv, 0.0);
	params.roughness = max(0.045, effectInfo.r);
	params.metalness = effectInfo.g;
	params.ao = effectInfo.b;

	return params;
}
