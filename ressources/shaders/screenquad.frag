#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ;

// Uniform: the texture.
uniform sampler2D screenTexture;
uniform float time;

// Output: the fragment color
out vec3 fragColor;

// Functions for colorspace conversion taken from http://lolengine.net/blog/2013/07/27/rgb-to-hsv-in-glsl

vec3 rgb2hsv(vec3 c)
{
	vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
	vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
	vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
	
	float d = q.x - min(q.w, q.y);
	float e = 1.0e-10;
	return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c)
{
	vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
	vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
	return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main(){
	
	vec3 colorRGB = texture(screenTexture,In.uv).rgb;
	vec3 colorHSV = rgb2hsv(colorRGB);
	// Shift the hue.
	colorHSV.x += time;
	fragColor = hsv2rgb(colorHSV);
}
