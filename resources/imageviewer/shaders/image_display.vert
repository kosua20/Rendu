
out INTERFACE {
	vec2 uv; ///< Texture coordinates.
} Out ;

uniform float screenRatio; ///< Screen h/v ratio.
uniform float imageRatio; ///< Image h/v ratio.
uniform float widthRatio; ///< Image/screen width ratio.
uniform vec2 flipAxis; ///< Denotes if a flipping has been applied on each axis.
uniform vec2 angleTrig; ///< Applied rotation precomputed cosine and sine.
uniform float pixelScale; ///< Scaling.
uniform vec2 mouseShift; ///< Translation.

/**
 Generate one triangle covering the whole screen and compute UV coordinates based on scaling/position/rotation.
 */
void main(){
	vec2 temp = 2.0 * vec2(gl_VertexID == 1, gl_VertexID == 2);
	gl_Position.xy = 2.0 * temp - 1.0;
	gl_Position.zw = vec2(1.0);
	// Center uvs and scale/translate.
	vec2 uv = pixelScale * gl_Position.xy - mouseShift*vec2(2.0,-2.0);
	
	// Image and screen ratio corrections.
	uv *= vec2(imageRatio, screenRatio);
	uv *= widthRatio;
	
	// Rotation.
	float nx = angleTrig.x*uv.x-angleTrig.y*uv.y;
	float ny = angleTrig.x*uv.y+angleTrig.y*uv.x;
	uv = vec2(nx,ny);
	
	// Flipping
	uv  *= 1.0 - flipAxis * 2.0;
	Out.uv = uv * 0.5 + 0.5;
}
