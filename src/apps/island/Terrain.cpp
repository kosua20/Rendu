#include "Terrain.hpp"


Terrain::Terrain(uint resolution, uint seed) : _resolution(resolution), _seed(seed) {
	generateMesh();
	generateMap();
}

void Terrain::generateMesh(){

	const int numLevels = _mshOpts.levels;
	const int baseLevelTexelSize = _mshOpts.size;
	const int elementCount = baseLevelTexelSize/2;
	std::vector<glm::vec3> positions;
	std::vector<uint> indices;
	uint baseId = 0;
	for(int lid = 0; lid < numLevels; ++lid){
		// Size of a basic square.
		const int currSize = 1 << lid;
		const int currHalfSize = currSize/2;
		const int prevSize = std::max(1 << (lid-1), 0);

		const int rad = currSize * (elementCount + 1);

		for(int z = -rad; z < rad; z += currSize){
			for(int x = -rad; x < rad; x += currSize){
				// Check if the current point falls inside one of the previous levels.
				if(std::max(std::abs(x + currHalfSize), std::abs(z + currHalfSize)) < elementCount * prevSize){
					continue;
				}
				// Compute positions for corners, edge centers and square center.
				const glm::vec3 pNW(x, lid, z);
				const glm::vec3 pNE(x + currSize, lid, z);
				const glm::vec3 pSW(x, lid, z + currSize);
				const glm::vec3 pSE(x + currSize, lid, z + currSize);
				const glm::vec3 pN = 0.5f * (pNW + pNE);
				const glm::vec3 pS = 0.5f * (pSW + pSE);
				const glm::vec3 pW = 0.5f * (pNW + pSW);
				const glm::vec3 pE = 0.5f * (pNE + pSE);
				const glm::vec3 pC = 0.5f * (pNW + pSE);
				positions.push_back(pNW);
				positions.push_back(pN);
				positions.push_back(pNE);
				positions.push_back(pW);
				positions.push_back(pC);
				positions.push_back(pE);
				positions.push_back(pSW);
				positions.push_back(pS);
				positions.push_back(pSE);


				// Special processing at borders.
				if(x == -rad){
					// We do a larger triangle to connect to the next level.
					indices.push_back(baseId + 4);
					indices.push_back(baseId + 0);
					indices.push_back(baseId + 6);
				} else {
					indices.push_back(baseId + 4);
					indices.push_back(baseId + 0);
					indices.push_back(baseId + 3);
					indices.push_back(baseId + 4);
					indices.push_back(baseId + 3);
					indices.push_back(baseId + 6);
				}

				if(z == -rad){
					// We do a larger triangle to connect to the next level.
					indices.push_back(baseId + 4);
					indices.push_back(baseId + 2);
					indices.push_back(baseId + 0);
				} else {
					indices.push_back(baseId + 4);
					indices.push_back(baseId + 2);
					indices.push_back(baseId + 1);
					indices.push_back(baseId + 4);
					indices.push_back(baseId + 1);
					indices.push_back(baseId + 0);
				}

				if(x + currSize >= rad){
					// We do a larger triangle to connect to the next level.
					indices.push_back(baseId + 4);
					indices.push_back(baseId + 8);
					indices.push_back(baseId + 2);
				} else {
					indices.push_back(baseId + 4);
					indices.push_back(baseId + 8);
					indices.push_back(baseId + 5);
					indices.push_back(baseId + 4);
					indices.push_back(baseId + 5);
					indices.push_back(baseId + 2);
				}

				if(z + currSize >= rad){
					// We do a larger triangle to connect to the next level.
					indices.push_back(baseId + 4);
					indices.push_back(baseId + 6);
					indices.push_back(baseId + 8);
				} else {
					indices.push_back(baseId + 4);
					indices.push_back(baseId + 6);
					indices.push_back(baseId + 7);
					indices.push_back(baseId + 4);
					indices.push_back(baseId + 7);
					indices.push_back(baseId + 8);
				}

				baseId += 9;
			}
		}

	}
	_mesh.clean();
	_mesh.positions = positions;
	_mesh.indices = indices;
	_mesh.upload();
}

void Terrain::generateMap(){

	Random::seed(_seed);

	Image heightMap(_resolution, _resolution, 1);
	// Generate FBM noise with multiple layers of Perlin noise.
	_perlin.generateLayers(heightMap, _genOpts.octaves, _genOpts.gain, _genOpts.lacunarity, _genOpts.scale);

	// Adjust to create the island overall shape and scale.
	const float invSize = 1.0f/float(heightMap.width);
	for(uint y = 0; y < heightMap.height; ++y){
		for(uint x = 0; x < heightMap.width; ++x){
			// Compute UV.
			const glm::vec2 uv = 2.0f * invSize * glm::vec2(x,y) - 1.0f;
			const float dst2 = glm::dot(uv, uv);
			const float scale = _genOpts.rescale * std::pow(std::max(1.0f - dst2, 0.0f), _genOpts.falloff);
			const float val = heightMap.r(x,y);
			heightMap.r(x,y) = _genOpts.maxHeight * (scale * (val + 1.0f) - 1.0f);
		}
	}
	// Then smooth to avoid pinches.
	Image dst(heightMap.width, heightMap.height, 1);
	for(int y = 0; y < int(heightMap.height); ++y){
		for(int x = 0; x < int(heightMap.width); ++x){
			const int xm = std::max(x-1, 0);
			const int xp = std::min(x+1, int(heightMap.width)-1);
			const int ym = std::max(y-1, 0);
			const int yp = std::min(y+1, int(heightMap.height)-1);
			const float & rN = heightMap.r(xm, y);
			const float & rS = heightMap.r(xp, y);
			const float & rW = heightMap.r(x, ym);
			const float & rE = heightMap.r(x, yp);
			const float & r  = heightMap.r(x,y);
			const float newHeight = 0.35f * r + 0.25f * 0.65f * (rN + rS + rW + rE);
			dst.r(x,y) = newHeight;
		}
	}
	for(int y = 0; y < int(_map.height); ++y){
		for(int x = 0; x < int(_map.width); ++x){
			heightMap.r(x,y) = dst.r(x,y);
		}
	}

	// Erosion.
	if(_erOpts.apply){
		erode(heightMap);
	}

	// Compute normals and mips.
	transferAndUpdateMap(heightMap);

}

void Terrain::erode(Image & img){

	const glm::ivec2 maxPos = glm::ivec2(img.width-1);
	for(int did = 0; did < _erOpts.dropsCount; ++did){
		// Draw a point at random.
		glm::vec2 pos(Random::Float(0.0f, maxPos[0]), Random::Float(0.0f, maxPos[1]));
		glm::vec2 dir(0.0f, 0.0f);
		float velocity = 1.0f;
		float water = 1.0f;
		float sediment = 0.0f;

		for(int sid = 0; sid < _erOpts.stepsMax; ++sid){
			if(water < 0.00001f){
				break;
			}
			// Gradient computation based on the four surrounding texels.
			glm::ivec2 ipos = glm::floor(pos); // nodeXY
			ipos = glm::clamp(ipos, glm::ivec2(0), maxPos);
			const glm::ivec2 inpos = glm::min(ipos+1, maxPos);

			const glm::vec2 dpos = pos - glm::vec2(ipos); // cellOffset
			const float h00 = img.r( ipos[0],  ipos[1]);
			const float h10 = img.r(inpos[0],  ipos[1]);
			const float h01 = img.r( ipos[0], inpos[1]);
			const float h11 = img.r(inpos[0], inpos[1]);
			const glm::vec2 grad((h10 - h00) * (1.0f - dpos.y) + (h11 - h01) * (dpos.y),
								 (h01 - h00) * (1.0f - dpos.x) + (h11 - h10) * (dpos.x));

			// We go down the slope, with some inertia.
			dir = _erOpts.inertia * dir - (1.0f - _erOpts.inertia) * grad;
			if(dir[0] != 0.0f || dir[1] != 0.0f){
				dir = glm::normalize(dir);
			}

			const glm::vec2 oldPos = pos;
			pos += dir;

			if((dir[0] == 0.0f && dir[1] == 0.0f) || pos[0] < 0.0f || pos[1] < 0.0f || pos[0] >= maxPos[0] || pos[1] >= maxPos[1]){
				break;
			}
			const float oldHeight = h00 * (1.0f - dpos.x) * (1.0f - dpos.y) + h10 * (1.0f - dpos.y) * dpos.x + h01 * (1.0f - dpos.x) * dpos.y + h11 * dpos.x * dpos.y;
			float newHeight = oldHeight;
			{
				glm::ivec2 nipos = glm::floor(pos);
				nipos = glm::clamp(nipos, glm::ivec2(0), maxPos);
				const glm::ivec2 ninpos = glm::min(nipos+1, maxPos);
				const glm::vec2 ndpos = pos - glm::vec2(nipos);
				const float nh00 = img.r( nipos[0],  nipos[1]);
				const float nh10 = img.r(ninpos[0],  nipos[1]);
				const float nh01 = img.r( nipos[0], ninpos[1]);
				const float nh11 = img.r(ninpos[0], ninpos[1]);
				newHeight = nh00 * (1.0f - ndpos.x) * (1.0f - ndpos.y) + nh10 * (1.0f - ndpos.y) * ndpos.x + nh01 * (1.0f - ndpos.x) * ndpos.y + nh11 * ndpos.x * ndpos.y;
			}

			const float dHeight = newHeight - oldHeight;
			const float capacity = std::max(-dHeight, _erOpts.minSlope) * velocity * water * _erOpts.capacityBase;

			if(sediment > capacity || dHeight > 0.0){
				// Deposit at the old location.
				const float deposit = dHeight > 0.0 ? std::min(sediment, dHeight) : ((sediment - capacity) * _erOpts.deposition);
				sediment -= deposit;
				img.r( ipos[0],  ipos[1]) += (1.0f - dpos.x) * (1.0f - dpos.y) * deposit;
				img.r( ipos[0], inpos[1]) += (1.0f - dpos.x) * (dpos.y) * deposit;
				img.r(inpos[0],  ipos[1]) += (dpos.x) * (1.0f - dpos.y) * deposit;
				img.r(inpos[0], inpos[1]) += (dpos.x) * (dpos.y) * deposit;
			} else {

				// Take some from the old location surroundings.
				float gather = std::min((capacity - sediment) * _erOpts.erosion, -dHeight);
				sediment += gather;
				float total = 0.0f;
				const int rad = _erOpts.gatherRadius;
				const int tsize = 2*rad+1;
				std::vector<std::vector<float>> wis(tsize, std::vector<float>(tsize));
				for(int dy = -rad; dy <= rad; ++dy){
					for(int dx = -rad; dx <= rad; ++dx){
						const float wi = std::max(0.0f, rad - glm::distance(glm::vec2(ipos[0]+dx, ipos[1]+dy), oldPos));
						total += wi;
						wis[dy+rad][dx+rad] = wi;
					}
				}
				for(int dy = -rad; dy <= rad; ++dy){
					for(int dx = -rad; dx <= rad; ++dx){
						const glm::ivec2 nnpos = ipos + glm::ivec2(dx, dy);
						if(nnpos[0] < 0 || nnpos[1] < 0 || nnpos[0] > maxPos[0] || nnpos[1] > maxPos[1]){
							continue;
						}
						img.r(nnpos[0], nnpos[1]) -= gather * wis[dy+rad][dx+rad]/total;
					}
				}

			}
			water *= (1.0 - _erOpts.evaporation);
			velocity = std::sqrt(std::max(0.0f, velocity*velocity + dHeight * _erOpts.gravity));
		}

	}
}

void Terrain::transferAndUpdateMap(Image & heightMap){

	_map.width = _map.height = _resolution;
	_map.levels = _map.depth = 1;
	_map.shape = TextureShape::D2;
	_map.clean();

	_map.images.emplace_back(_map.width, _map.height, 4);
	const glm::ivec2 maxPos = glm::ivec2(_map.width-1);
	const int rad = 4;
	const float dWorld = 2.0f * float(rad) * _texelSize;
	for(uint y = 0; y < _map.height; ++y){
		for(uint x = 0; x < _map.width; ++x){
			// Compute normal using smooth finite differences.
			glm::vec2 dh(0.0f);
			float total = 0.0f;
			for(int ds = -2; ds < 2; ++ds){
				const float weight = 1.0f / (std::abs(ds) + 1.0f);
				total += weight;
				for(int dds = 1; dds <= rad; ++dds){

					const glm::ivec2 pixXp = glm::clamp(glm::ivec2(x + dds, y + ds), glm::ivec2(0), maxPos);
					const float heightXp = heightMap.r(pixXp[0], pixXp[1]);
					const glm::ivec2 pixXm = glm::clamp(glm::ivec2(x - dds, y - ds), glm::ivec2(0), maxPos);
					const float heightXm = heightMap.r(pixXm[0], pixXm[1]);
					dh[0] += weight * (heightXp - heightXm);

					const glm::ivec2 pixZp = glm::clamp(glm::ivec2(x + ds, y + dds), glm::ivec2(0), maxPos);
					const float heightZp = heightMap.r(pixZp[0], pixZp[1]);
					const glm::ivec2 pixZm = glm::clamp(glm::ivec2(x - ds, y - dds), glm::ivec2(0), maxPos);
					const float heightZm = heightMap.r(pixZm[0], pixZm[1]);
					dh[1] += weight * (heightZp - heightZm);
				}
			}
			dh /= (float(rad) * total);

			glm::vec3 n = glm::cross(glm::vec3(0.0f, dh[1], dWorld), glm::vec3(dWorld, dh[0], 0.0f));
			n = glm::normalize(n);

			_map.images[0].rgba(x,y) = glm::vec4(heightMap.r(x,y), n);

		}
	}
	
	// Build mipmaps.
	_map.levels = _map.getMaxMipLevel();
	std::array<float, 9> weights = {1.0f/16.0f, 1.0f/8.0f, 1.0f/16.0f, 1.0f/8.0f, 1.0f/4.0f, 1.0f/8.0f, 1.0f/16.0f, 1.0f/8.0f, 1.0f/16.0f};
	for(uint lid = 1; lid < _map.levels; ++lid){
		const uint w = _map.width / (1 << lid);
		const uint h = _map.height / (1 << lid);
		_map.images.emplace_back(w, h, 4);
		const glm::ivec2 maxPos(w*2-1, h*2-1);
		Image & currImg = _map.images[lid];
		Image & prevImg = _map.images[lid-1];
		for(uint y = 0; y < h; ++y){
			for(uint x = 0; x < w; ++x){
				const glm::vec2 prevCoords = 2.0f * glm::vec2(x,y) - 0.5f;
				glm::vec4 total(0.0f);
				for(int dy = -1; dy <= 1; ++dy){
					for(int dx = -1; dx <= 1; ++dx){
						glm::ivec2 coords = glm::ivec2(prevCoords + glm::vec2(dx, dy));
						coords = glm::clamp(coords, glm::ivec2(0), maxPos);
						const float & weight = weights[3*dy + dx];
						total += weight * prevImg.rgba(coords[0], coords[1]);
					}
				}
				currImg.rgba(x,y) = total;
			}
		}
	}
	
	// Send to the GPU.
	_map.upload({Layout::RGBA32F, Filter::LINEAR_LINEAR, Wrap::CLAMP}, false);
}

void Terrain::interface(){

	if(ImGui::TreeNode("Mesh")){
		ImGui::InputInt("Grid size", &_mshOpts.size);
		ImGui::InputInt("Grid levels", &_mshOpts.levels);

		if(ImGui::Button("Update mesh")){
			generateMesh();
		}
		ImGui::TreePop();
	}

	bool dirtyTerrain = false;
	bool dirtyErosion = false;

	if(ImGui::TreeNode("Perlin FBM")){
		dirtyTerrain = ImGui::InputInt("Resolution", &_resolution) || dirtyTerrain;
		dirtyTerrain = ImGui::InputInt("Octaves", &_genOpts.octaves) || dirtyTerrain;
		dirtyTerrain = ImGui::SliderFloat("Lacunarity", &_genOpts.lacunarity, 0.0f, 10.0f) || dirtyTerrain;
		dirtyTerrain = ImGui::SliderFloat("Gain", &_genOpts.gain, 0.0f, 1.0f) || dirtyTerrain;
		dirtyTerrain = ImGui::SliderFloat("Scale", &_genOpts.scale, 0.0f, 0.1f) || dirtyTerrain;
		dirtyTerrain = ImGui::SliderFloat("Max height", &_genOpts.maxHeight, 1.0f, 10.0f) || dirtyTerrain;
		dirtyTerrain = ImGui::SliderFloat("Falloff", &_genOpts.falloff, 1.0f, 10.0f) || dirtyTerrain;
		dirtyTerrain = ImGui::SliderFloat("Rescale", &_genOpts.rescale, 0.5f, 3.0f) || dirtyTerrain;
		ImGui::TreePop();
	}

	if(ImGui::TreeNode("Erosion")){
		dirtyErosion = ImGui::Checkbox("Apply erosion", &_erOpts.apply) || dirtyErosion;
		dirtyErosion = ImGui::InputInt("Drops count", &_erOpts.dropsCount) || dirtyErosion;
		dirtyErosion = ImGui::InputInt("Drop step", &_erOpts.stepsMax) || dirtyErosion;
		dirtyErosion = ImGui::InputInt("Gather radius", &_erOpts.gatherRadius) || dirtyErosion;
		dirtyErosion = ImGui::SliderFloat("Inertia", &_erOpts.inertia, 0.0f, 1.0f) || dirtyErosion;
		dirtyErosion = ImGui::SliderFloat("Gravity", &_erOpts.gravity, 2.0f, 18.0f) || dirtyErosion;
		dirtyErosion = ImGui::SliderFloat("Min Slope", &_erOpts.minSlope, 0.0f, 0.1f) || dirtyErosion;
		dirtyErosion = ImGui::SliderFloat("Capacity Base", &_erOpts.capacityBase, 2.0f, 16.0f) || dirtyErosion;
		dirtyErosion = ImGui::SliderFloat("Erosion", &_erOpts.erosion, 0.0f, 1.0f) || dirtyErosion;
		dirtyErosion = ImGui::SliderFloat("Evaporation", &_erOpts.evaporation, 0.0f, 0.1f) || dirtyErosion;
		dirtyErosion = ImGui::SliderFloat("Deposition", &_erOpts.deposition, 0.0f, 1.0f) || dirtyErosion;
		ImGui::TreePop();
	}
	if(dirtyTerrain || dirtyErosion){
		generateMap();
	}
}

void Terrain::clean() {
	_map.clean();
	_mesh.clean();
}
