#version 400

in INTERFACE {
	vec3 dir;
	vec2 uv; ///< Texture coordinates.
} In ;

uniform float iTime;
uniform float iTimeDelta;
uniform float iFrame;
uniform vec3 iResolution;
uniform vec4 iMouse;
uniform mat4 iView;
uniform mat4 iProj;
uniform mat4 iViewProj;
uniform mat4 iViewInv;
uniform mat4 iProjInv;
uniform mat4 iViewProjInv;
uniform mat4 iNormalMat;
uniform vec3 iCamPos;
uniform vec3 iCamUp;
uniform vec3 iCamCenter;
uniform float iCamFov;

layout(binding = 0) uniform sampler2D previousFrame;
layout(binding = 1) uniform sampler2D sdfFont;

layout(location = 0) out vec4 fragColor; ///< Color.


uniform vec3 bgCol0 = vec3(0.308,0.066,0.327);
uniform vec3 bgCol1 = vec3(0.131,0.204,0.458);
uniform vec3 triCol0 = vec3(0.957,0.440,0.883);
uniform vec3 triCol1 = vec3(0.473,0.548,0.919);
uniform vec3 col4 = vec3(0.987,0.746,0.993);
uniform vec3 col5 = vec3(0.033,0.011,0.057);
uniform vec3 col6 = vec3(0.633,0.145,0.693);
uniform vec3 col7 = vec3(0.977,1.000,1.000);
uniform vec3 col8 = vec3(0.024,0.811,0.924);
uniform vec3 col9 = vec3(0.600,0.960,1.080);
uniform vec3 col10 = vec3(0.494,0.828,0.977);
uniform vec3 col11 = vec3(0.968,0.987,0.999);

uniform int gridX = 10;
uniform int gridY = 20;
uniform vec4 cornerPos = vec4(0.5, 0.37, 0.0, 0.0);
uniform float gridHeight = 0.3;
uniform float vignetteScale = 1.0;
uniform bool showLetters = true;
uniform bool showGrid = true;


/*
	ShaderWave - A recreation of a 80's nostalgia-fueled meme image.
	Simon Rodriguez, 2019.
	Feel free to reuse this code for any non-commercial purpose.
*/

/// Noise helpers.

// 1-D noise.
float noise(float p){
	float fl = floor(p);
	float fc = fract(p);
	float rand0 = fract(sin(fl) * 43758.5453123);
	float rand1 = fract(sin(fl+1.0) * 43758.5453123);
	return mix(rand0, rand1, fc);
}

// 4 channels 1-D noise.
vec4 noise(vec4 p){
	vec4 fl = floor(p);
	vec4 fc = fract(p);
	vec4 rand0 = fract(sin(fl) * 43758.5453123);
	vec4 rand1 = fract(sin(fl+1.0) * 43758.5453123);
	return mix(rand0, rand1, fc);
}

// 2D to 1D hash, by Dave Hoskins (https://www.shadertoy.com/view/4djSRW)
float hash(vec2 p){
	vec3 p3  = fract(vec3(p.xyx) * 0.2831);
	p3 += dot(p3, p3.yzx + 19.19);
	return fract((p3.x + p3.y) * p3.z);
}


/// Background utilities.

// Generate starfield.
float stars(vec2 localUV, float starsDens, float starsDist){
	// Cenetr and scale UVs.
	vec2 p = (localUV-0.5) * starsDist;
	// Use thresholded high-frequency noise.
	float brigthness = smoothstep(1.0 - starsDens, 1.0, hash(floor(p)));
	// Apply soft transition between the stars and the background.
	const float startsTh = 0.5;
	return smoothstep(startsTh, 0.0, length(fract(p) - 0.5)) * brigthness;
}

// Distance from point to line segment.
float segmentDistance(vec2 p, vec2 a, vec2 b){
	// Project the point on the segment.
	vec2 dir = b - a;
	float len2 = dot(dir, dir);
	float t = clamp(dot(p-a, dir)/len2,0.0,1.0);
	vec2 proj = a + t * dir;
	// Distance between the point and its projection.
	return distance(p, proj);
}

// Distance from point to triangle edges.
float triangleDistance(vec2 p, vec4 tri, float width){
	// Point at the bottom center, shared by all triangles.
	vec2 pogridX = cornerPos.xy;
	// Distance to each segment.
	float minDist = 	   segmentDistance(p, pogridX, tri.xy) ;
	minDist = min(minDist, segmentDistance(p, tri.xy, tri.zw));
	minDist = min(minDist, segmentDistance(p, tri.zw, pogridX));
	// Smooth result for transition.
	return 1.0-smoothstep(0.0, width, minDist);
}

/// Text utilities.

float getLetter(int lid, vec2 uv){
	// If outside, return arbitrarily high distance.
	if(uv.x < 0.0 || uv.y < 0.0 || uv.x > 1.0 || uv.y > 1.0){
		return 1000.0;
	}
	// The font texture is 16x16 glyphs.
	int vlid = lid/16;
	int hlid = lid - 16*vlid;
	vec2 fontUV = (vec2(hlid, vlid) + uv)/16.0;
	// Fetch in a 3x3 neighborhood to box blur
	float accum = 0.0;
	for(int i = -1; i < 2; ++i){
		for(int j = -1; j < 2; ++j){
			vec2 offset = vec2(i,j)/1024.0;
			accum += texture(sdfFont, fontUV+offset, 0.0).a;
		}
	}
	return accum/9.0;
}

vec3 textGradient(float interior, float top, vec2 alphas){
	// Use squared blend for the interior gradients.
	vec2 alphas2 = alphas*alphas;
	// Generate the four possible gradients (interior/edge x upper/lower)
	vec3 bottomInterior = mix(col4, col5, alphas2.x);
	vec3 bottomExterior = mix(col6, col7,  alphas.x);
	vec3 topInterior 	= mix(col8, col9, alphas2.y);
	vec3 topExterior 	= mix(col10, col11,  alphas.y);
	// Blend based on current location.
	vec3 gradInterior 	= mix(bottomInterior, topInterior, top);
	vec3 gradExterior 	= mix(bottomExterior, topExterior, top);
	return mix(gradExterior, gradInterior, interior);
}


/// Main render.

void main(){
	// Normalized pixel coordinates.
	vec2 uvCenter = 2.0 * In.uv - 1.0;
	uvCenter *= max(iResolution.xy/iResolution.yx, 1.0);
	vec2 uv = 0.5*uvCenter+0.5;

	/// Background.
	// Color gradient.
	vec3 finalColor = 1.5*mix(bgCol0, bgCol1, uv.x);

	if(showGrid && uv.y < gridHeight){

		/// Bottom grid.
		// Compute local cflipped oordinates for the grid.
		vec2 localUV = uv*vec2(2.0, -1.0/gridHeight) + vec2(-1.0, 1.0);
		// Perspective division, scaling, foreshortening and alignment.
        localUV.x = localUV.x/(localUV.y+0.8);
		localUV *= vec2(gridX, gridY);
		localUV.y = sqrt(localUV.y);
		localUV.x += 0.5;
		// Generate grid smooth lines (translate along time).
		vec2 unitUV = fract(localUV-vec2(0.0, 0.3*iTime));
		vec2 gridAxes = smoothstep(0.02, 0.07, unitUV) * (1.0 - smoothstep(0.93, 0.98, unitUV));
		float gridAlpha = 1.0-clamp(gridAxes.x*gridAxes.y, 0.0, 1.0);

		/// Fixed star halos.
		// Loop UVs.
		vec2 cyclicUV = mod(localUV-vec2(0.0, 0.3*iTime), vec2(9.0, 5.0));
		// Distance to some fixed grid vertices.
		const float haloTh = 0.6;
		float isBright1 = 1.0-min(distance(cyclicUV, vec2(6.0,3.0)), haloTh)/haloTh;
		float isBright2 = 1.0-min(distance(cyclicUV, vec2(1.0,2.0)), haloTh)/haloTh;
		float isBright3 = 1.0-min(distance(cyclicUV, vec2(3.0,4.0)), haloTh)/haloTh;
		float isBright4 = 1.0-min(distance(cyclicUV, vec2(2.0,1.0)), haloTh)/haloTh;
		// Halos brightness.
		float spotLight = isBright1+isBright2+isBright3+isBright4;
		spotLight *= spotLight;
		// Composite grid lines and halos.
		finalColor += 0.15*gridAlpha*(1.0+5.0*spotLight);

	} else {
		/// Starfield.
		// Compensate aspect ratio for circular stars.
		vec2 ratioUVs = uv;
		// Decrease density towards the bottom of the screen.
		float baseDens = clamp(uv.y-0.3, 0.0, 1.0);
		// Three layers of stars with varying density, cyclic animation.
        float deltaDens = 20.0*(sin(0.05*iTime-1.5)+1.0);
		finalColor += 0.50*stars(ratioUVs, 0.10*baseDens, 150.0-deltaDens);
		finalColor += 0.75*stars(ratioUVs, 0.05*baseDens,  80.0-deltaDens);
		finalColor += 1.00*stars(ratioUVs, 0.01*baseDens,  30.0-deltaDens);
	}

	/// Triangles.
	// Triangles upper points.
	vec4 points1 = vec4(0.30,0.85,0.70,0.85);
	vec4 points2 = vec4(0.33,0.83,0.73,0.88);
	vec4 points3 = vec4(0.35,0.80,0.66,0.82);
	vec4 points4 = vec4(0.38,0.91,0.66,0.87);
	vec4 points5 = vec4(0.31,0.89,0.72,0.83);
	// Randomly perturb based on time.
	points2 += 0.04*noise(10.0*points2+0.4*iTime);
	points3 += 0.04*noise(10.0*points3+0.4*iTime);
	points4 += 0.04*noise(10.0*points4+0.4*iTime);
	points5 += 0.04*noise(10.0*points5+0.4*iTime);
	// Intensity of the triangle edges.
	float tri1 = triangleDistance(uv, points1, 0.010);
	float tri2 = triangleDistance(uv, points2, 0.005);
	float tri3 = triangleDistance(uv, points3, 0.030);
	float tri4 = triangleDistance(uv, points4, 0.005);
	float tri5 = triangleDistance(uv, points5, 0.003);
	float intensityTri = 0.9*tri1+0.5*tri2+0.2*tri3+0.6*tri4+0.5*tri5;
	// Triangles color gradient, from left to right.
	float alphaTriangles = clamp((uv.x-0.3)/0.4, 0.0, 1.0);
	vec3 baseTriColor = mix(triCol0, triCol1.rgb, alphaTriangles);
	// Additive blending.
	finalColor += intensityTri*baseTriColor;

	/// Horizon gradient.
	const float horizonHeight = 0.025;
	float horizonIntensity = 1.0-min(abs(uv.y - gridHeight), horizonHeight)/horizonHeight;
	// Modulate base on distance to screen edges.
	horizonIntensity *= (1.0 - 0.7*abs(uvCenter.x)+0.5);
	finalColor += 2.0*horizonIntensity*baseTriColor;

	/// Letters.
	// Centered UVs for text box.
	vec2 textUV = uvCenter * 2.2 * vec2(0.7, 1.0) - vec2(0.0, 0.6);
	if(showLetters && abs(textUV.x) < 1.0 && abs(textUV.y) < 1.0){
		// Rescale UVs.
		textUV = textUV*vec2(1.75,0.5)+vec2(1.63,0.5);
		// Per-sign UV, manual shifts for kerning.
		const vec2 letterScaling = vec2(0.47,0.93);
		vec2 uvLetter1 = (textUV - vec2(0.60,0.50)) * letterScaling + 0.5;
		vec2 uvLetter2 = (textUV - vec2(1.50,0.50)) * letterScaling + 0.5;
		vec2 uvLetter3 = (textUV - vec2(2.15,0.54)) * letterScaling * 1.2 + 0.5;
		vec2 uvLetter4 = (textUV - vec2(2.70,0.50)) * letterScaling + 0.5;
		// Get letters distance to edge, merge.
		float let1 = getLetter(200, uvLetter1);
		float let2 = getLetter(192, uvLetter2);
		float let3 = getLetter(215, uvLetter3);
		float let4 = getLetter(131, uvLetter4);
		// Merge and threshold.
		float finalDist = 0.52 - min(let1, min(let2, min(let3, let4)));
		// Split between top and bottom gradients (landscape in the reflection).
		float localTh = 0.49+0.03*noise(70.0*uv.x+iTime);
		float isTop = smoothstep(localTh-0.01, localTh+0.01, textUV.y);
		// Split between interior and edge gradients.
		float isInt = smoothstep(0.018, 0.022, finalDist);
		// Compute relative heights along the color gradients (both bottom and up (shifted)), rescale.
		vec2 localUBYs = vec2(1.8*(textUV.y-0.5)+0.5);
		localUBYs.y -= isTop*0.5;
		vec2 gradientBlend = localUBYs / localTh;
		// Evaluate final mixed color gradient.
		vec3 textColor = textGradient(isInt, isTop, gradientBlend);
		// Add sharp reflection along a flat diagonal.
		if(textUV.x-20.0*textUV.y < -14.0 || textUV.x-20.0*textUV.y > -2.5){
			textColor += 0.1;
		}
		// Soft letter edges.
		float finalDistSmooth = smoothstep(-0.0025, 0.0025,finalDist);
		finalColor = mix(finalColor, textColor, finalDistSmooth);
	}

	/// Vignetting.
	const float radiusMin = 0.8;
	const float radiusMax = 1.8;
	float vignetteIntensity = vignetteScale*(length(uvCenter)-radiusMin)/(radiusMax-radiusMin);
	finalColor *= clamp(1.0-vignetteIntensity, 0.0, 1.0);

	/// Exposure tweak, output.
	fragColor = vec4(pow(finalColor, vec3(1.2)),1.0);
}
