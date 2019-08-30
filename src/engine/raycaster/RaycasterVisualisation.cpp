#include "raycaster/RaycasterVisualisation.hpp"
#include <queue>
#include <stack>

RaycasterVisualisation::RaycasterVisualisation(const Raycaster & raycaster) :
	_raycaster(raycaster) {
}

void RaycasterVisualisation::getAllLevels(std::vector<Mesh> & meshes) const {
	std::vector<DisplayNode> selectedNodes;

	// Breadth-first tree exploration.
	std::queue<DisplayNode> nodesToVisit;
	// Start by visiting each object.
	for(size_t nid = 0; nid < _raycaster._meshCount; ++nid) {
		nodesToVisit.push({nid, 0});
	}

	while(!nodesToVisit.empty()) {
		const DisplayNode location = nodesToVisit.front();
		selectedNodes.push_back(location);
		// Remove the current node from the visit queue.
		nodesToVisit.pop();
		// If this is not a leaf, enqueue the two children nodes.
		const Raycaster::Node & node = _raycaster._hierarchy[location.node];
		if(!node.leaf) {
			nodesToVisit.push({node.left, location.depth + 1});
			nodesToVisit.push({node.right, location.depth + 1});
		}
	}

	createBVHMeshes(selectedNodes, meshes);
}

Raycaster::RayHit RaycasterVisualisation::getRayLevels(const glm::vec3 & origin, const glm::vec3 & direction, std::vector<Mesh> & meshes, float mini, float maxi) const {

	const Raycaster::Ray ray(origin, direction);
	std::vector<DisplayNode> selectedNodes;
	std::stack<DisplayNode> nodesToTest;
	// Start by testing each object.
	for(size_t nid = 0; nid < _raycaster._meshCount; ++nid) {
		// If the ray doesn't intersect the bounding box, move to the next node.
		if(!Raycaster::intersects(ray, _raycaster._hierarchy[nid].box, mini, maxi)) {
			continue;
		}
		nodesToTest.push({nid, 0});
	}
	Raycaster::RayHit bestHit;
	while(!nodesToTest.empty()) {
		const DisplayNode & infos	= nodesToTest.top();
		const Raycaster::Node & node = _raycaster._hierarchy[infos.node];
		const size_t depth			 = infos.depth;
		selectedNodes.push_back(infos);
		nodesToTest.pop();

		// If the node is a leaf, test all included triangles.
		if(node.leaf) {
			for(size_t tid = 0; tid < node.right; ++tid) {
				const auto & tri			= _raycaster._triangles[node.left + tid];
				const Raycaster::RayHit hit = _raycaster.intersects(ray, tri, mini, maxi);
				// We found a valid hit.
				if(hit.hit && hit.dist < bestHit.dist) {
					bestHit			   = hit;
					maxi			   = bestHit.dist;
					bestHit.internalId = ulong(node.left) + ulong(tid);
				}
			}
			// Move to the next node.
			continue;
		}
		// Else, intersect both child nodes.
		if(Raycaster::intersects(ray, _raycaster._hierarchy[node.left].box, mini, maxi)) {
			nodesToTest.push({node.left, depth + 1});
		}
		if(Raycaster::intersects(ray, _raycaster._hierarchy[node.right].box, mini, maxi)) {
			nodesToTest.push({node.right, depth + 1});
		}
	}
	createBVHMeshes(selectedNodes, meshes);
	return bestHit;
}

void RaycasterVisualisation::getRayMesh(const glm::vec3 & rayPos, const glm::vec3 & rayDir, const Raycaster::RayHit & hit, Mesh & mesh, float defaultLength) const {
	const float length	 = hit.hit ? hit.dist : defaultLength;
	const glm::vec3 hitPos = rayPos + length * glm::normalize(rayDir);
	// Ray color: green if hit, red otherwise.
	const glm::vec3 rayColor(hit.hit ? 0.0f : 1.0f, hit.hit ? 1.0f : 0.0f, 0.0f);
	// Create the geometry.
	mesh.clean();
	mesh.positions = {rayPos, hitPos};
	mesh.colors	= {rayColor, rayColor};
	mesh.indices   = {0, 1, 0};
	// If there was a hit, add the intersected triangle to the visualisation.
	if(hit.hit) {
		const Raycaster::TriangleInfos & tri = _raycaster._triangles[hit.internalId];
		const glm::vec3 & v0				 = _raycaster._vertices[tri.v0];
		const glm::vec3 & v1				 = _raycaster._vertices[tri.v1];
		const glm::vec3 & v2				 = _raycaster._vertices[tri.v2];
		mesh.positions.push_back(v0);
		mesh.positions.push_back(v1);
		mesh.positions.push_back(v2);
		mesh.colors.push_back(rayColor);
		mesh.colors.push_back(rayColor);
		mesh.colors.push_back(rayColor);
		mesh.indices.push_back(2);
		mesh.indices.push_back(3);
		mesh.indices.push_back(4);
	}
}

void RaycasterVisualisation::createBVHMeshes(const std::vector<DisplayNode> & nodes, std::vector<Mesh> & meshes) const {
	// Compute the max depth.
	size_t maxDepth = 0;
	for(const auto & displayNode : nodes) {
		maxDepth = std::max(maxDepth, displayNode.depth);
	}
	meshes.clear();
	meshes.resize(maxDepth + 1);
	// Setup degenerate triangles for each line of a cube.
	const std::vector<unsigned int> indices = {
		0, 1, 0, 0, 2, 0, 1, 3, 1, 2, 3, 2, 4, 5, 4, 4, 6, 4, 5, 7, 5, 6, 7, 6, 1, 5, 1, 0, 4, 0, 2, 6, 2, 3, 7, 3};

	// Generate the geometry for all nodes.
	for(const auto & displayNode : nodes) {
		const Raycaster::Node & node = _raycaster._hierarchy[displayNode.node];
		// Setup vertices.
		Mesh & mesh					  = meshes[displayNode.depth];
		const unsigned int firstIndex = uint(mesh.positions.size());
		const auto corners			  = node.box.getCorners();
		for(const auto & corner : corners) {
			mesh.positions.push_back(corner);
		}
		for(const unsigned int iid : indices) {
			mesh.indices.push_back(firstIndex + iid);
		}
	}

	// Associate a color to all the nodes at a given depth.
	for(size_t did = 0; did < maxDepth + 1; ++did) {
		// Compute relative depth for colorisation.
		float depth = float(did) / float(maxDepth);
		// We have fewer boxes at low depth, skew the hue scale.
		depth *= depth;
		// Decrease value as we go deeper.
		const float val		  = 0.5f * (1.0f - depth) + 0.25f;
		const glm::vec3 color = glm::rgbColor(glm::vec3(300.0f * depth, 1.0f, val));
		Mesh & mesh			  = meshes[did];
		const size_t vCount   = mesh.positions.size();
		mesh.colors			  = std::vector<glm::vec3>(vCount, color);
	}
}
