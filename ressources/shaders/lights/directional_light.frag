#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ;

// Uniforms
uniform sampler2D albedoTexture;
uniform sampler2D normalTexture;
uniform sampler2D depthTexture;
uniform sampler2D effectsTexture;

uniform vec2 inverseScreenSize;
uniform vec4 projectionMatrix;
uniform mat4 inverseV;

uniform vec3 lightDirection;//(direction in view space)
uniform vec3 lightColor;

// Output: the fragment color
out vec3 fragColor;

vec3 positionFromDepth(float depth){
	float depth2 = 2.0 * depth - 1.0 ;
	vec2 ndcPos = 2.0 * In.uv - 1.0;
	// Linearize depth -> in view space.
	float viewDepth = - projectionMatrix.w / (depth2 + projectionMatrix.z);
	// Compute the x and y components in view space.
	return vec3(- ndcPos * viewDepth / projectionMatrix.xy , viewDepth);
}


// Compute the light shading.

vec3 shading(vec3 diffuseColor, vec3 n, vec3 v, float specularCoeff){
	
	// Compute the direction from the point to the light
	vec3 d = normalize(lightDirection);
	
	// Compute the diffuse factor
	float diffuse = max(0.0, dot(d,n));
	
	// Compute the specular factor
	float specular = 0.0;
	if(diffuse > 0.0){
		vec3 r = reflect(-d,n);
		specular = pow(max(dot(r,v),0.0),64.0);
		specular *= specularCoeff;
	}
	
	return diffuse * diffuseColor * lightColor + specular * lightColor;
}


void main(){
	
	vec4 albedo =  texture(albedoTexture,In.uv);
	// If this is the skybox, don't shade.
	if(albedo.a == 0.0){
		discard;
	}
	
	vec3 diffuseColor = albedo.rgb;
	vec3 n = 2.0 * texture(normalTexture,In.uv).rgb - 1.0;
	float depth = texture(depthTexture,In.uv).r;
	float specularCoeff = texture(effectsTexture,In.uv).g;
	
	vec3 v = normalize(-positionFromDepth(depth));
	
	vec3 lightShading = shading(diffuseColor, n, v, specularCoeff);
	fragColor.rgb = lightShading;
}

