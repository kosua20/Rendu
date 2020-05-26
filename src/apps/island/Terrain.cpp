#include "Terrain.hpp"


Terrain::Terrain(uint resolution, uint seed) : _resolution(resolution), _seed(seed) {
	generateMesh();
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

void Terrain::interface(){

	if(ImGui::TreeNode("Mesh")){
		ImGui::InputInt("Grid size", &_mshOpts.size);
		ImGui::InputInt("Grid levels", &_mshOpts.levels);

		if(ImGui::Button("Update mesh")){
			generateMesh();
		}
		ImGui::TreePop();
	}
}

void Terrain::clean() {
	_map.clean();
	_mesh.clean();
}
