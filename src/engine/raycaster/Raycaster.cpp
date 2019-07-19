#include "Raycaster.hpp"
#include "helpers/Random.hpp"
#include "helpers/System.hpp"
#include <queue>
#include <stack>

Raycaster::Ray::Ray(const glm::vec3 & origin, const glm::vec3 & direction) : pos(origin), dir(glm::normalize(direction)){
}

Raycaster::RayHit::RayHit() : hit(false), dist(std::numeric_limits<float>::max()), u(0.0f), v(0.0f), localId(0), meshId(0) {
}

Raycaster::RayHit::RayHit(float distance, float uu, float vv, unsigned long lid, unsigned long mid) :
	hit(true), dist(distance), u(uu), v(vv), w(1.0f - uu - vv), localId(lid), meshId(mid) {
}

Raycaster::Raycaster(){
	
}

void Raycaster::addMesh(const Mesh & mesh, const glm::mat4 & model){
	const unsigned long indexOffset = (unsigned long)(_vertices.size());
	
	// Start by copying all vertices.
	const size_t startIndex = _vertices.size();
	_vertices.insert(_vertices.end(), mesh.positions.begin(), mesh.positions.end());
	const size_t endIndex = _vertices.size();
	
	if(model != glm::mat4(1.0f)){
		for(size_t vid = startIndex; vid < endIndex; ++vid){
			_vertices[vid] = glm::vec3(model * glm::vec4(_vertices[vid], 1.0f));
		}
	}
	
	const size_t startTriangle = _triangles.size();
	const size_t trianglesCount = mesh.indices.size()/3;
	for(size_t tid = 0; tid < trianglesCount; ++tid){
		const size_t localId = 3 * tid;
		TriangleInfos triInfos;
		triInfos.v0 = indexOffset + mesh.indices[localId + 0];
		triInfos.v1 = indexOffset + mesh.indices[localId + 1];
		triInfos.v2 = indexOffset + mesh.indices[localId + 2];
		triInfos.localId = (unsigned long)(localId);
		triInfos.meshId = _meshCount;
		triInfos.box = BoundingBox(_vertices[triInfos.v0], _vertices[triInfos.v1], _vertices[triInfos.v2]);
		_triangles.push_back(triInfos);
	}
	
	// Add initial node to the hierarchy.
	_hierarchy.emplace_back();
	Node & node = _hierarchy.back();
	node.left = startTriangle;
	node.right = trianglesCount;
	
	Log::Info() << "[Raycaster]" << " Mesh " << _meshCount << " added, " << trianglesCount << " triangles, " << _vertices.size() - indexOffset << " vertices." << std::endl;
	
	++_meshCount;
}

void Raycaster::updateHierarchy(){
	
	Log::Info() << "[Raycaster] Building hierarchy for " << _triangles.size() << " triangles... " << std::flush;
	
	struct SetInfos {
		size_t id;
		size_t begin;
		size_t count;
	};
	
	std::stack<SetInfos> remainingSets;
	// One root node per mesh.
	for(size_t nid = 0; nid < _meshCount; ++nid){
		const Node & node = _hierarchy[nid];
		remainingSets.push({nid, node.left, node.right});
	}
	
	while(!remainingSets.empty()){
		// Get the next node to process on the stack.
		const SetInfos current(remainingSets.top());
		remainingSets.pop();
		const size_t begin = current.begin;
		const size_t count = current.count;
		
		// Compute the global bounding box.
		BoundingBox global(_triangles[begin].box);
		for(size_t tid = 1; tid < count; ++tid){
			global.merge(_triangles[begin+tid].box);
		}
		
		Node & node = _hierarchy[current.id];
		node.box = global;
		
		// If the triangles count is low enough, we have a leaf.
		if(count < 3){
			node.leaf = true;
			node.left = begin;
			node.right = count;
			
		} else {
			node.leaf = false;
			size_t splitCount = 0;
			// Pick the dimension along which the global bounding box is the largest.
			const glm::vec3 boxSize = global.getSize();
			const int axis = (boxSize.x >= boxSize.y && boxSize.x >= boxSize.z) ? 0 : (boxSize.y >= boxSize.z ? 1 : 2);
			
			if(count >= 5){
				
				// Compute the midpoint of all triangles centroids along the picked axis.
				float abscisse = 0.0f;
				for(size_t tid = 0; tid < count; ++tid){
					abscisse += _triangles[begin+tid].box.getCentroid()[axis];
				}
				abscisse /= float(count);
				
				// Split in two subnodes.
				// Main criterion: split at the midpoint along the chosen axis.
				const auto split = std::partition(_triangles.begin() + begin, _triangles.begin() + begin + count, [abscisse, axis](const TriangleInfos & t0){
					return t0.box.getCentroid()[axis] < abscisse;
				});
				splitCount = std::distance(_triangles.begin() + begin, split);
			}
			
			// Fallback criterion: split in two equal size subsets.
			// This can happen in case the primitive boxes overlap a lot,
			// or in case of equal coordinates along the chosen axis.
			if(splitCount == 0 || splitCount == count || count < 5){
				splitCount = count/2;
				std::nth_element(_triangles.begin() + begin, _triangles.begin() + begin + splitCount, _triangles.begin() + begin + count, [axis](const TriangleInfos & t0, const TriangleInfos & t1){
					return t0.box.getCentroid()[axis] < t1.box.getCentroid()[axis];
				});
			}
			
			// Create the left and right sub-nodes.
			// Can't use the currentNode reference anymore because of emplace_back.
			// Left:
			_hierarchy.emplace_back();
			const size_t leftPos = _hierarchy.size()-1;
			_hierarchy[current.id].left = leftPos;
			remainingSets.push({leftPos, begin, splitCount});
			// Right:
			_hierarchy.emplace_back();
			const size_t rightPos = _hierarchy.size()-1;
			_hierarchy[current.id].right = rightPos;
			remainingSets.push({rightPos, begin+splitCount, count-splitCount });
			
		}
	}
	Log::Info() << " Done: " << _hierarchy.size() << " nodes created." << std::endl;
	
}

void Raycaster::createBVHMeshes(std::vector<Mesh> &meshes) const {
	meshes.clear();
	// We want a mesh where the nodes are in increasing order of depth.
	struct NodeLocation {
		size_t node;
		size_t depth;
	};
	
	std::vector<NodeLocation> sortedNodes;
	sortedNodes.reserve(_hierarchy.size());
	
	// Breadth-first tree exploration.
	std::queue<NodeLocation> nodesToVisit;
	// Start by visiting each object.
	for(size_t nid = 0; nid < _meshCount; ++nid){
		nodesToVisit.push({nid, 0});
	}
	size_t maxDepth = 0;
	while(!nodesToVisit.empty()){
		const NodeLocation location = nodesToVisit.front();
		sortedNodes.push_back(location);
		// If this is not a leaf, enqueue the two children nodes.
		const Node & node = _hierarchy[location.node];
		if(!node.leaf){
			nodesToVisit.push({ node.left , location.depth+1 });
			nodesToVisit.push({ node.right, location.depth+1 });
		}
		// Find the max depth.
		maxDepth = std::max(maxDepth, location.depth);
		// Remove the current node from the visit queue.
		nodesToVisit.pop();
	}
	
	meshes.resize(maxDepth+1);
	
	// For each node, generate a wireframe bounding box.
	for(const auto & location : sortedNodes){
		const Node & node = _hierarchy[location.node];
		
		// Compute relative depth for colorisation.
		float depth = float(location.depth) / float(maxDepth);
		// We have fewer boxes at low depth, skew the hue scale.
		depth *= depth;
		// Decrease luminosity as we go deeper.
		const float lum = 0.5f*(1.0f - depth);
		const glm::vec3 color = System::hslToRgb(glm::vec3(300.0f*depth, 0.9f, lum));
		
		// Setup vertices.
		Mesh & mesh = meshes[location.depth];
		const unsigned int firstIndex = (unsigned int)mesh.positions.size();
		const auto corners = node.box.getCorners();
		for(const auto & corner : corners){
			mesh.positions.push_back(corner);
			mesh.colors.push_back(color);
		}
		// Setup degenerate triangles for each line.
		const std::vector<unsigned int> indices = {
			0,1,0, 0,2,0, 1,3,1, 2,3,2, 4,5,4, 4,6,4, 5,7,5, 6,7,6, 1,5,1, 0,4,0, 2,6,2, 3,7,3
		};
		for(const unsigned int iid : indices){
			mesh.indices.push_back(firstIndex + iid);
		}
	}
}

const Raycaster::RayHit Raycaster::intersects(const glm::vec3 & origin, const glm::vec3 & direction, float mini, float maxi) const {
	const Ray ray(origin, direction);
	
	std::stack<size_t> nodesToTest;
	// Start by testing each object.
	for(size_t nid = 0; nid < _meshCount; ++nid){
		nodesToTest.push(nid);
	}
	
	RayHit bestHit;
	while(!nodesToTest.empty()){
		const Node & node = _hierarchy[nodesToTest.top()];
		nodesToTest.pop();
		
		// If the ray doesn't intersect the bounding box, move to the next node.
		if(!Raycaster::intersects(ray, node.box, mini, maxi)){
			continue;
		}
		// If the node is a leaf, test all included triangles.
		if(node.leaf){
			for(size_t tid = 0; tid < node.right; ++tid){
				const auto & tri = _triangles[node.left + tid];
				const RayHit hit = intersects(ray, tri, mini, maxi);
				// We found a valid hit.
				if(hit.hit && hit.dist < bestHit.dist){
					bestHit = hit;
					maxi = bestHit.dist;
				}
			}
			// Move to the next node.
			continue;
		}
		// Else, intersect both child nodes.
		nodesToTest.push(node.left);
		nodesToTest.push(node.right);
	}
	return bestHit;
}

bool Raycaster::intersectsAny(const glm::vec3 & origin, const glm::vec3 & direction, float mini, float maxi) const {
	const Ray ray(origin, direction);
	
	std::stack<size_t> nodesToTest;
	// Start by testing each object.
	for(size_t nid = 0; nid < _meshCount; ++nid){
		nodesToTest.push(nid);
	}
	
	while(!nodesToTest.empty()){
		const Node & node = _hierarchy[nodesToTest.top()];
		nodesToTest.pop();
		// If the ray doesn't intersect the bounding box, move to the next node.
		if(!Raycaster::intersects(ray, node.box, mini, maxi)){
			continue;
		}
		// If the node is a leaf, test all included triangles.
		if(node.leaf){
			RayHit finalHit;
			for(size_t tid = 0; tid < node.right; ++tid){
				const auto & tri = _triangles[node.left + tid];
				if(intersects(ray, tri, mini, maxi).hit){
					return true;
				}
			}
			// No intersection move to the next node.
			continue;
		}
		// Check if any of the children is hit.
		nodesToTest.push(node.left);
		nodesToTest.push(node.right);
	}
	return false;
}

bool Raycaster::visible(const glm::vec3 & p0, const glm::vec3 & p1) const {
	const glm::vec3 direction = p1 - p0;
	const float maxi = glm::length(direction);
	return !intersectsAny(p0, direction, 0.0001f, maxi);
}

const Raycaster::RayHit Raycaster::intersects(const Raycaster::Ray & ray, const TriangleInfos & tri, float mini, float maxi) const {
	// Implement Moller-Trumbore intersection test.
	const glm::vec3 & v0 = _vertices[tri.v0];
	const glm::vec3 v01 = _vertices[tri.v1] - v0;
	const glm::vec3 v02 = _vertices[tri.v2] - v0;
	const glm::vec3 p = glm::cross(ray.dir, v02);
	const float det = glm::dot(v01, p);
	
	if(std::abs(det) < std::numeric_limits<float>::epsilon()){
		return RayHit();
	}
	
	const float invDet = 1.0f / det;
	const glm::vec3 q = ray.pos - v0;
	const float u = invDet * glm::dot(q, p);
	if(u < 0.0f || u > 1.0f){
		return RayHit();
	}
	
	const glm::vec3 r = glm::cross(q, v01);
	const float v = invDet * glm::dot(ray.dir, r);
	if(v < 0.0f || (u+v) > 1.0f){
		return RayHit();
	}
	
	const float t = invDet * glm::dot(v02, r);
	if(t > mini && t < maxi){
		return RayHit(t, u, v, tri.localId, tri.meshId);
	}
	return RayHit();
}

bool Raycaster::intersects(const Raycaster::Ray & ray, const BoundingBox & box, float mini, float maxi){
	const glm::vec3 minRatio = (box.minis - ray.pos) / ray.dir;
	const glm::vec3 maxRatio = (box.maxis - ray.pos) / ray.dir;
	const glm::vec3 minFinal = glm::min(minRatio, maxRatio);
	const glm::vec3 maxFinal = glm::max(minRatio, maxRatio);
	
	const float closest  = std::max(minFinal[0], std::max(minFinal[1], minFinal[2]));
	const float furthest = std::min(maxFinal[0], std::min(maxFinal[1], maxFinal[2]));
	
	return std::max(closest, mini) <= std::min(furthest, maxi);
}

glm::vec3 Raycaster::interpolatePosition(const RayHit & hit, const Mesh & geometry){
	const unsigned long triId = hit.localId;
	const unsigned long i0 = geometry.indices[triId  ];
	const unsigned long i1 = geometry.indices[triId+1];
	const unsigned long i2 = geometry.indices[triId+2];
	return hit.w * geometry.positions[i0] + hit.u * geometry.positions[i1] + hit.v * geometry.positions[i2];
}

glm::vec3 Raycaster::interpolateNormal(const RayHit & hit, const Mesh & geometry){
	const unsigned long triId = hit.localId;
	const unsigned long i0 = geometry.indices[triId  ];
	const unsigned long i1 = geometry.indices[triId+1];
	const unsigned long i2 = geometry.indices[triId+2];
	const glm::vec3 n = hit.w * geometry.normals[i0] + hit.u * geometry.normals[i1] + hit.v * geometry.normals[i2];
	return glm::normalize(n);
}

glm::vec2 Raycaster::interpolateUV(const RayHit & hit, const Mesh & geometry){
	const unsigned long triId = hit.localId;
	const unsigned long i0 = geometry.indices[triId  ];
	const unsigned long i1 = geometry.indices[triId+1];
	const unsigned long i2 = geometry.indices[triId+2];
	return hit.w * geometry.texcoords[i0] + hit.u * geometry.texcoords[i1] + hit.v * geometry.texcoords[i2];
}
