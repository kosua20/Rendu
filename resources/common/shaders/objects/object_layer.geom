#version 400
layout (triangles) in; ///< Triangles as input.
layout (triangle_strip, max_vertices = 18) out; ///< Output 6 triangles.

uniform mat4 vps[6]; ///< The viewproj matrices.

in GS_INTERFACE {
	vec4 pos; ///< World position.
	vec2 uv; ///< UV coordinates.
} In[];

out vec3 worldPos; ///< Pass the world space position along.
out vec2 uv; ///< UV coordinates.
/**
 Emit transformed geometry for each face of the cubemap, applying the corresponding view-projection transformation.
 */
void main() {
	for(int i = 0; i < 6; ++i){
		// For each face of the cubemap, we emit a transformed triangle.
		// We pass the world position and UVs to the fragment shader.
		gl_Layer = i;
		
		gl_PrimitiveID = gl_PrimitiveIDIn;
		worldPos = In[0].pos.xyz;
		uv = In[0].uv;
		gl_Position = vps[i] * In[0].pos;
		EmitVertex();
		
		gl_PrimitiveID = gl_PrimitiveIDIn;
		worldPos = In[1].pos.xyz;
		uv = In[1].uv;
		gl_Position = vps[i] * In[1].pos;
		EmitVertex();
		
		gl_PrimitiveID = gl_PrimitiveIDIn;
		worldPos = In[2].pos.xyz;
		uv = In[2].uv;
		gl_Position = vps[i] * In[2].pos;
		EmitVertex();
		
		EndPrimitive();
	}
}
