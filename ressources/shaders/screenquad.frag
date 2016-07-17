#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ;

// Uniforms: the texture, inverse of the screen size, FXAA flag.
uniform sampler2D screenTexture;
uniform vec2 inverseScreenSize;
uniform bool useFXAA;

// Settings for FXAA.
#define EDGE_THRESHOLD_MIN 0.0312
#define EDGE_THRESHOLD_MAX 0.125

// Output: the fragment color
out vec3 fragColor;

// Return the luma value in perceptual space for a given RGB color in linear space.
float rgb2luma(vec3 rgb){
	return sqrt(dot(rgb, vec3(0.299, 0.587, 0.114)));
}

void main(){
	
	vec3 colorCenter = texture(screenTexture,In.uv).rgb;
	
	// If AA disabled, return directly the fragment color.
	if(!useFXAA){
		fragColor = colorCenter;
		return;
	}
	
	// Display a small green square in the bottom-left corner if AA is enabled.
	if(max(gl_FragCoord.x, gl_FragCoord.y) < 20){
		fragColor = vec3(0.2,0.8,0.4);
		return;
	}
	
	// Luma at the current fragment
	float lumaCenter = rgb2luma(colorCenter);
	
	// Luma at the four direct neighbours of the current fragment.
	float lumaDown = rgb2luma(textureOffset(screenTexture,In.uv,ivec2(0,-1)).rgb);
	float lumaUp = rgb2luma(textureOffset(screenTexture,In.uv,ivec2(0,1)).rgb);
	float lumaLeft = rgb2luma(textureOffset(screenTexture,In.uv,ivec2(-1,0)).rgb);
	float lumaRight = rgb2luma(textureOffset(screenTexture,In.uv,ivec2(1,0)).rgb);
	
	// Find the maximum and minimum luma around the current fragment.
	float lumaMin = min(lumaCenter,min(min(lumaDown,lumaUp),min(lumaLeft,lumaRight)));
	float lumaMax = max(lumaCenter,max(max(lumaDown,lumaUp),max(lumaLeft,lumaRight)));
	
	// Compute the delta.
	float lumaRange = lumaMax - lumaMin;
	
	// If the luma variation is lower that a threshold (or if we are in a really dark area), we are not on an edge, don't perform any AA.
	if(lumaRange < max(EDGE_THRESHOLD_MIN,lumaMax*EDGE_THRESHOLD_MAX)){
		fragColor = colorCenter;
		return;
	}
	
	// Query the 4 remaining corners lumas.
	float lumaDownLeft = rgb2luma(textureOffset(screenTexture,In.uv,ivec2(-1,-1)).rgb);
	float lumaUpRight = rgb2luma(textureOffset(screenTexture,In.uv,ivec2(1,1)).rgb);
	float lumaUpLeft = rgb2luma(textureOffset(screenTexture,In.uv,ivec2(-1,1)).rgb);
	float lumaDownRight = rgb2luma(textureOffset(screenTexture,In.uv,ivec2(1,-1)).rgb);
	
	// Combine the four edges lumas (using intermediary variables for future computations with the same values).
	float lumaDownUp = lumaDown + lumaUp;
	float lumaLeftRight = lumaLeft + lumaRight;
	
	// Same for corners
	float lumaLeftCorners = lumaDownLeft + lumaUpLeft;
	float lumaDownCorners = lumaDownLeft + lumaDownRight;
	float lumaRightCorners = lumaDownRight + lumaUpRight;
	float lumaUpCorners = lumaUpRight + lumaUpLeft;
	
	// Compute an estimation of the gradient along the horizontal and vertical axis.
	float edgeHorizontal =	abs(-2.0 * lumaLeft + lumaLeftCorners)	+ abs(-2.0 * lumaCenter + lumaDownUp ) * 2.0	+ abs(-2.0 * lumaRight + lumaRightCorners);
	float edgeVertical =	abs(-2.0 * lumaUp + lumaUpCorners)		+ abs(-2.0 * lumaCenter + lumaLeftRight) * 2.0	+ abs(-2.0 * lumaDown + lumaDownCorners);
	
	// Is the local edge horizontal or vertical ?
	bool isHorizontal = (edgeHorizontal >= edgeVertical);
	
	// Choose the step size (one pixel) accordingly.
	float stepLength = isHorizontal ? inverseScreenSize.y : inverseScreenSize.x;
	
	// Select the two neighboring texels lumas in the opposite direction to the local edge.
	float luma1 = isHorizontal ? lumaDown : lumaLeft;
	float luma2 = isHorizontal ? lumaUp : lumaRight;
	// Compute gradients in this direction.
	float gradient1 = luma1 - lumaCenter;
	float gradient2 = luma2 - lumaCenter;
	
	// Which direction is the steepest ?
	bool is1Steepest = abs(gradient1) >= abs(gradient2);
	
	// Gradient in the corresponding direction, normalized.
	float gradientScaled = 0.25*max(abs(gradient1),abs(gradient2));
	
	// Average luma in the correct direction.
	float lumaLocalAverage = 0.0;
	if(is1Steepest){
		// Switch the direction
		stepLength = - stepLength;
		lumaLocalAverage = 0.5*(luma1 + lumaCenter);
	} else {
		lumaLocalAverage = 0.5*(luma2 + lumaCenter);
	}
	
	// Shift UV in the correct direction by half a pixel.
	vec2 currentUv = In.uv;
	if(isHorizontal){
		currentUv.y += stepLength * 0.5;
	} else {
		currentUv.x += stepLength * 0.5;
	}
	
	fragColor = vec3(10000.0*(currentUv - In.uv), 1.0);
}
