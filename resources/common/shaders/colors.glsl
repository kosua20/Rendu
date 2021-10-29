#include "constants.glsl"

/** Convert a temperature color to an RGB triplet.
Based on the analytical fit described by Neil Bartlett in his 
color-temperature package (https://github.com/neilbartlett/color-temperature).
\param tempK the color temperature in Kelvins
\return a corresponding RGB color. 
*/
vec3 temperatureToRgb(float tempK){
	
	float tempS = tempK/100.0;
	vec3 rgb = vec3(0.0);

	if(tempS < 66.0){
		// Fit red.
		rgb.r = 1.0;
		// Fit green.
		float green = tempS - 2.0;
		rgb.g = -0.44596950469579133/255.0 * green + 104.49216199393888/255.0 * log(green) - 155.25485562709179/255.0;
		// Fit blue.
		if(tempS <= 20.0){
			rgb.b = 0.0;
		} else {
			float blue = tempS - 10.0;
			rgb.b = 0.8274096064007395/255.0 * blue + 115.67994401066147/255.0 * log(blue) -254.76935184120902/255.0;
		}
		
	} else {
		// Fit red.
		float red = tempS - 55.0;
		rgb.r = 0.114206453784165/255.0 * red - 40.25366309332127/255.0 * log(red) + 351.97690566805693/255.0;
		// Fit green.
		float green = tempS - 50.0;
		rgb.g = 0.07943456536662342/255.0 * green - 28.0852963507957/255.0 * log(green) + 325.4494125711974/255.0;
		// Fit blue.
		rgb.b = 1.0;
	}

	return rgb;
}

/** Convert a RGB color value to a HSV color.
\param rgb the RGB color to convert
\return the HSV triplet, where the hue is in degrees.
*/
vec3 rgbToHsv(vec3 rgb){
	float minC = min(rgb.r, min(rgb.g, rgb.b));
	float maxC = max(rgb.r, max(rgb.g, rgb.b));
	// Black.
	if(maxC == 0.0){
		return vec3(0.0);
	}

	float delta = maxC - minC;
	vec3 hsv;
	hsv.z = maxC;
	hsv.y = delta / maxC;

	// Hue circle, in degrees.
	float h;
	if(rgb.r == maxC){
		h =   0.0 + 60.0 / delta * (rgb.g - rgb.b);
	} else if(rgb.g == maxC){
		h = 120.0 + 60.0 / delta * (rgb.b - rgb.r);
	} else {
		h = 240.0 + 60.0 / delta * (rgb.r - rgb.g);
	}

	hsv.x = h < 0.0 ? (h + 360.0) : h;
	return hsv;
}

/** Convert a HSV color value to a RGB color.
\param hsv the HSV color to convert, where the hue is in degrees.
\return the RGB triplet
*/
vec3 hsvToRgb(vec3 hsv){
	// Grey.
	if(hsv.y == 0.0){
		return vec3(hsv.z);
	}
	float sector = floor(hsv.x / 60.0);
	float frac = hsv.x / 60.0 - sector;

	vec3 fracts = vec3(1.0, frac, 1.0 - frac);
	vec3 opq = hsv.z * (1.0 - hsv.y * fracts);
	
	switch(int(sector)){
		default:
		case 0:
			return vec3(hsv.z, opq.z, opq.x);
		case 1:
			return vec3(opq.y, hsv.z, opq.x);
		case 2:
			return vec3(opq.x, hsv.z, opq.z);
		case 3:
			return vec3(opq.x, opq.y, hsv.z);
		case 4:
			return vec3(opq.z, opq.x, hsv.z);
		case 5:
			return vec3(hsv.z, opq.x, opq.y);
	}

}

/** Convert a XYZ value (CIE 1931 RGB color space with neutral E illuminant) to a RGB value
 * \param xyz the XYZ color to convert
 * \return the RGB triplet
 */
vec3 xyzToRgb(vec3 xyz){
	return mat3(2.3706743, -0.5138850, 0.0052982, -0.9000405, 1.4253036, -0.0146949, -0.4706338, 0.0885814, 1.0093968) * xyz;
}

/** Evalute the luma value in perceptual space for a given RGB color in linear space.
\param rgb the input RGB color
\return the perceptual luma
*/
float rgb2luma(vec3 rgb){
	return sqrt(dot(rgb, vec3(0.299, 0.587, 0.114)));
}
