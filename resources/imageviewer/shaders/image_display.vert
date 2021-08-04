layout(location = 0) in vec3 v;///< Position.

layout(location = 0) out INTERFACE {
	vec2 uv; ///< Texture coordinates.
} Out ;

layout(binding = 0) uniform UniformBlock {
	vec2 flipAxis; ///< Denotes if a flipping has been applied on each axis.
	vec2 angleTrig; ///< Applied rotation precomputed cosine and sine.
	vec2 mouseShift; ///< Translation.
	float screenRatio; ///< Screen h/v ratio.
	float imageRatio; ///< Image h/v ratio.
	float widthRatio; ///< Image/screen width ratio.
	float pixelScale; ///< Scaling.
};

/**
 Generate one triangle covering the whole screen and compute UV coordinates based on scaling/position/rotation.
 */
void main(){
	gl_Position.xy = v.xy;
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


	gl_Position.y *= -1;
}
