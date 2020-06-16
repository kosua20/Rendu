#version 400

#include "constants.glsl"

in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In ;

layout(binding = 0) uniform sampler1D tex1D; ///< Image to output.
layout(binding = 1) uniform sampler1DArray tex1DArray; ///< Image to output.
layout(binding = 2) uniform sampler2D tex2D; ///< Image to output.
layout(binding = 3) uniform sampler2DArray tex2DArray; ///< Image to output.
layout(binding = 4) uniform samplerCube texCube; ///< Image to output.
layout(binding = 5) uniform samplerCubeArray texCubeArray; ///< Image to output.
layout(binding = 6) uniform sampler3D tex3D; ///< Image to output.

uniform ivec4 channels;
uniform vec2 range;
uniform int level;
uniform int layer;
uniform bool gamma;

uniform int shape;

layout(location = 0) out vec3 fragColor; ///< Color.

#define TextureShapeD1 (1<<1)
#define TextureShapeD2 (1<<2)
#define TextureShapeD3 (1<<3)
#define TextureShapeCube (1<<4)
#define TextureShapeArray (1<<5)
#define TextureShapeArray1D (TextureShapeD1 | TextureShapeArray)
#define TextureShapeArray2D (TextureShapeD2 | TextureShapeArray)
#define TextureShapeArrayCube (TextureShapeCube | TextureShapeArray)


/** Just pass the input image as-is, potentially performing up/down scaling. */
void main(){
	vec4 color = vec4(1.0,0.5,0.0,0.5);

	if(shape == TextureShapeD1){
		color = textureLod(tex1D, In.uv.x, level);
	} else if(shape == TextureShapeArray1D){
		color = textureLod(tex1DArray, vec2(In.uv.x, layer), level);
	} else if(shape == TextureShapeD2){
		color = textureLod(tex2D, In.uv, level);
	} else if(shape == TextureShapeArray2D){
		color = textureLod(tex2DArray, vec3(In.uv, layer), level);
	} else if(shape == TextureShapeD3){
		float depth = textureSize(tex3D, 0).z;
		color = textureLod(tex3D, vec3(In.uv, float(layer)/depth), level);
	} else if(shape == TextureShapeCube || shape == TextureShapeArrayCube){
		// For now, equirectangular.
		vec2 angles = M_PI * vec2(2.0, 1.0) * (In.uv - vec2(0.0, 0.5));
		vec2 cosines = cos(angles);
		vec2 sines = sin(angles);
		vec3 dir = vec3(cosines.y * cosines.x, sines.y, cosines.y * sines.x);
		if(shape == TextureShapeCube){
			color = textureLod(texCube, dir, level);
		} else {
			color = textureLod(texCubeArray, vec4(dir, layer/6), level);
		}
		// Highlight edges.
		vec3 adir = abs(dir);
		adir /= max(adir.x, max(adir.y, adir.z));
		if((adir.x + adir.y + adir.z - min(adir.x, min(adir.y, adir.z))) > 1.97){
			color = vec4(100000.0);
		}
	}

	// Rescale based on range.
	color = (color - range.x)/(range.y - range.x);
	// Gamma correct RGB if required.
	if(gamma){
		color.rgb = pow(color.rgb, vec3(1.0/2.2));
	}
	// Apply channel restrictions.
	int sid = 0;
	int validChannel = -1;
	for(int i = 0; i < 4; ++i){
		if(channels[i] == 0){
			color[i] = (i == 3 ? 1.0 : 0.0);
		} else {
			++sid;
			validChannel = i;
		}
	}

	// If only one channel displayed, display it in grey and no alpha.
	if(sid == 1){
		color.rgb = vec3(color[validChannel]);
		color.a = 1.0;
	}

	// Generate background grid.
	ivec2 gridCoords = ivec2(gl_FragCoord.xy)/20;
	int gridId = int(mod(gridCoords.x + gridCoords.y,2));
	vec3 bg = gridId == 0 ? vec3(0.55) : vec3(0.45);

	fragColor = mix(bg, color.rgb, color.a);

}
