
layout(location = 0) in INTERFACE {
	vec3 pos; ///< Position.
} In;

layout(set = 1, binding = 0) uniform samplerCube screenTexture; ///< Image to blur.

layout(set = 0, binding = 0) uniform UniformBlock {
	vec3 up; 			///< Face vertical vector.
	vec3 right; 		///< Face horizontal vector.
	float invHalfSize;  ///< Inverse half size of the sampled texture.
};

layout(location = 0) out vec4 fragColor; ///< Color.

/** Approximate box blur for cubemaps, with a similar effect to the 2D versions.
  This is incorrect as the texel footprint is not taken into account when weighting.
 */
void main(){
	// We have to unroll the box blur loop manually.
	// We shift the fetch direction by moving on the face plane, which is at distance invHalfSize.
	// To do this we use the right and up vectors of the face as displacement directions.
	vec4 color;
	color  = textureLod(screenTexture, In.pos + invHalfSize * (-2 * right + -2 * up), 0.0);
	color += textureLod(screenTexture, In.pos + invHalfSize * (-2 * right          ), 0.0);
	color += textureLod(screenTexture, In.pos + invHalfSize * (-2 * right +  2 * up), 0.0);
	color += textureLod(screenTexture, In.pos + invHalfSize * (-1 * right + -1 * up), 0.0);
	color += textureLod(screenTexture, In.pos + invHalfSize * (-1 * right +  1 * up), 0.0);
	color += textureLod(screenTexture, In.pos + invHalfSize * (             -2 * up), 0.0);
	color += textureLod(screenTexture, In.pos                                       , 0.0);
	color += textureLod(screenTexture, In.pos + invHalfSize * (              2 * up), 0.0);
	color += textureLod(screenTexture, In.pos + invHalfSize * ( 1 * right + -1 * up), 0.0);
	color += textureLod(screenTexture, In.pos + invHalfSize * ( 1 * right +  1 * up), 0.0);
	color += textureLod(screenTexture, In.pos + invHalfSize * ( 2 * right + -2 * up), 0.0);
	color += textureLod(screenTexture, In.pos + invHalfSize * ( 2 * right          ), 0.0);
	color += textureLod(screenTexture, In.pos + invHalfSize * ( 2 * right +  2 * up), 0.0);

	fragColor = color / 13.0;
}
