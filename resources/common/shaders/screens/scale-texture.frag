
layout(location = 0) in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In ;

layout(set = 1, binding = 0) uniform sampler2D screenTexture; ///< Texture to overlay.

layout(set = 0, binding = 0) uniform UniformBlock {
	float scale = 1.0; ///< Scaling factor.
};

layout(location = 0) out vec4 fragColor; ///< Scene color.

/** Scale the texture values. */
void main(){
	
	vec4 col = textureLod(screenTexture, In.uv, 0.0);
	fragColor = scale * col;
}
