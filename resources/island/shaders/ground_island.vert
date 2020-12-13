
// Attributes
layout(location = 0) in vec3 v; ///< Position.

uniform mat4 mvp; ///< MVP transformation matrix.
uniform vec3 shift; ///< Terrain shift in world space.
uniform float texelSize; ///< Height map texel world size.
uniform float invMapSize; ///< Height map inverse size.
uniform float invGridSize; ///< Grid mesh inverse size.

layout(binding=0) uniform sampler2D heightMap; ///< Terrain height map, height in R, normals in GBA.

out INTERFACE {
	vec3 pos; ///< World position.
	vec2 uv; ///< Texture coordinates.
} Out ;

/** Apply the transformation to the input vertex.
  Compute the tangent-to-view space transformation matrix.
 */
void main(){

	// Compute discretization level to move grid in lockstep.
	float levelSize = exp2(v.y) * texelSize;
	vec3 worldPos = texelSize * v + round(shift/levelSize)*levelSize;

	// Compute LOD level to read from the heightmap.
	vec2 dpos = abs(worldPos.xz - shift.xz);
	float tsize = max(0.5, max(dpos.x, dpos.y) * 2.0 * invGridSize);
	float fetchLod = max(log2(tsize) - 0.75, 0.0);
	float baseLod = floor(fetchLod);
	float nextLod = baseLod + 1.0;
	float fracLod = fetchLod - baseLod;
	// Half texel offset, to only read inbetween texels.
	float halfTexNext = exp2(baseLod);
	float halfTexBase = halfTexNext*0.5;
	// Shifted UVs.
	vec2 uv = (worldPos.xz/texelSize);
	vec2 baseCoords = (uv + halfTexBase) * invMapSize + 0.5;
	vec2 nextCoords = (uv + halfTexNext) * invMapSize + 0.5;
	// Custom trilinear 
	float lowHeight = textureLod(heightMap, baseCoords, baseLod).r;
	float nextHeight = textureLod(heightMap, nextCoords, nextLod).r;
	// Final height and projected position.
	worldPos.y = mix(lowHeight, nextHeight, fracLod);
    gl_Position = mvp * vec4(worldPos, 1.0);
	Out.uv = (uv + 0.5) * invMapSize + 0.5;
	Out.pos = worldPos;
}
