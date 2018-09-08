#version 330
layout (triangles) in; ///< Triangles as input.
layout (triangle_strip, max_vertices = 18) out; ///< Output 6 triangles.

uniform mat4 vps[6]; ///< The viewproj matrices.

in GS_INTERFACE {
	vec4 pos;
} In[]; ///< vec4 pos;

out vec3 worldPos; ///< Pass the world space position along.

/**
 Emit transformed geometry for each face of the cubemap, applying the corresponding view-projection transformation.
 */
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
