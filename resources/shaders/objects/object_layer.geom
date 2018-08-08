#version 330
layout (triangles) in;
layout (triangle_strip, max_vertices = 18) out;

uniform mat4 vps[6];

in GS_INTERFACE {
	vec4 pos;
} In[];

out vec3 worldPos;

void main() {
	for(int i = 0; i < 6; ++i){
		// For each face of the cubemap, we emit a transformed triangle.
		// We pass the world position to the fragment shader.
		gl_Layer = i;
		
		gl_PrimitiveID = gl_PrimitiveIDIn;
		worldPos = In[0].pos.xyz;
		gl_Position = vps[i] * In[0].pos;
		EmitVertex();
		
		gl_PrimitiveID = gl_PrimitiveIDIn;
		worldPos = In[1].pos.xyz;
		gl_Position = vps[i] * In[1].pos;
		EmitVertex();
		
		gl_PrimitiveID = gl_PrimitiveIDIn;
		worldPos = In[2].pos.xyz;
		gl_Position = vps[i] * In[2].pos;
		EmitVertex();
		
		EndPrimitive();
	}
}
