
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
