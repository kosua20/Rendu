#include "samplers.glsl"
#include "utils.glsl"

#define MATERIAL_UNLIT 0 ///< Passthrough, non shaded material.
#define MATERIAL_STANDARD 1 ///< Basic PBR material (dielectric or metal).
#define MATERIAL_CLEARCOAT 2 ///< Clear coat & standard PBR material.
#define MATERIAL_EMISSIVE 3 ///< Emissive material with a specular dielectric layer.
#define MATERIAL_ANISOTROPIC 4 ///< Anisotropic GGX material.
#define MATERIAL_SHEEN 5 ///< Sheen material.
#define MATERIAL_IRIDESCENT 6 ///< Material with a thin layer creating interferences.
#define MATERIAL_SUBSURFACE 7 ///< Material with a thin layer creating interferences.

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

/**
 Convert the iridescence normalized values to the physical quantities.
 \param ior the normalized index of refraction
 \param thickness the normalized thickness
 \return a vector containing a refraction index in [1.2, 2.4] and a thickness in [300nm, 800nm]
 */
vec2 decodeIridescence(float ior, float thickness){
	return vec2(1.2 + ior * 1.2, 300.0 + 500.0 * thickness);
}

/** Store a RGB color on 12 bits, split into 10 and 2 bits for storage in the Gbuffer.
 \param rgb the color to pack
 \return the 10 and 2 bits normalized values encoding the color
 */
vec2 encodeRgbOn10Plus2Bits(vec3 rgb){
	uvec3 rgbColor = uvec3(rgb * 255.0);
	// Keep only the 4 highest bits.
	rgbColor = (rgbColor & 0xF0) >> 4;
	uint rgbPacked = rgbColor.r | (rgbColor.g << 4) | (rgbColor.b << 8);
	// Split it into 10 and 2 bits for storage along with the normal.
	float rgbPacked10 = float((rgbPacked & 0xFFFFFC) >> 2) / float((1 << 10) - 1);
	float rgbPacked2 = float((rgbPacked & 0x03)) / float((1<<2)-1);
	return vec2(rgbPacked10, rgbPacked2);
}

/** Decode a RGB color from a 10 and a 2 bits normalized values
 \param packed the packed values
 \return the decoded color
 */
vec3 decodeRgbFrom10Plus2Bits(vec2 packed){
	// Rebuild 12 bits color stored in 10 and 2 bits channels.
	uint rgbPacked10 = uint(packed.x * float((1 << 10) - 1));
	uint rgbPacked2 = uint(packed.y * float((1 << 2) - 1));
	uint rgbPacked = rgbPacked2 | (rgbPacked10 << 2);
	uvec3 rgbColor = uvec3(rgbPacked & 0xF, (rgbPacked >> 4) & 0xF, (rgbPacked >> 8) & 0xF);
	vec3 rgbFinal = vec3(rgbColor << 4) / 255.0;
	return rgbFinal;
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
	// Sheen
	float sheeness; ///< Sheen mixing factor.
	vec3 sheenColor; ///< Sheen tint color.
	float sheenRoughness; ///< Roughness of the sheen lobe.
	// Iridescence
	float filmIndex; ///< The thin film index of refraction.
	float filmThickness; ///< The thin film thickness in nanometers.
	// Subsurface scattering.
	vec3 subsurfaceTint; ///< The tint of the subsurface
	float subsurfaceThickness; ///< The thickness to use for the subsurface.
	float subsurfaceRoughness; ///< The roughness used for the lobe emulating the subsurface.
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
	params.sheeness = 0.0;
	params.sheenColor = vec3(0.0);
	params.sheenRoughness = 0.0;
	params.filmIndex = 1.5;
	params.filmThickness = 0.0;
	params.subsurfaceTint = vec3(0.0);
	params.subsurfaceThickness = 1.0;
	params.subsurfaceRoughness = 1.0;
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

	// Decode sheen if present.
	if(material.id == MATERIAL_SHEEN){
		// Rebuild 12 bits color stored in 10 and 2 bits channels.
		material.sheenColor = decodeRgbFrom10Plus2Bits(normalInfo.ba);
		// Sheen roughness.
		material.sheenRoughness = max(0.045, effectInfo.a);
		// We replaced the metalness by the sheen factor.
		material.sheeness = material.metalness;
		material.metalness = 0.0;
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

	// Iridescent parameters.
	if(material.id == MATERIAL_IRIDESCENT){
		vec2 decodedIridescence = decodeIridescence(normalInfo.b, effectInfo.a);
		material.filmIndex = decodedIridescence.x;
		material.filmThickness = decodedIridescence.y;
	}

	// Subsurface parameters.
	if(material.id == MATERIAL_SUBSURFACE){
		// Rebuild 12 bits color stored in 10 and 2 bits channels.
		material.subsurfaceTint = decodeRgbFrom10Plus2Bits(normalInfo.ba);
		material.subsurfaceRoughness = max(0.045, material.metalness);
		material.subsurfaceThickness = effectInfo.a;
		material.metalness = 0.0;
	}

	
	return material;
}
