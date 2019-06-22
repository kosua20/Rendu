#include "Raycaster.hpp"
#include "helpers/GenerationUtilities.hpp"

Raycaster::Ray::Ray(const glm::vec3 & origin, const glm::vec3 & direction) : pos(origin), dir(glm::normalize(direction)){
}

Raycaster::RayHit::RayHit() : hit(false), dist(std::numeric_limits<float>::max()), u(0.0f), v(0.0f), localId(0), meshId(0) {
}

Raycaster::RayHit::RayHit(float distance, float uu, float vv, unsigned long lid, unsigned long mid) :
	hit(true), dist(distance), u(uu), v(vv), localId(lid), meshId(mid) {
}

Raycaster::Raycaster(){
	
}

void Raycaster::addMesh(const Mesh & mesh){
	const size_t indexOffset = _vertices.size();
	
	// Start by copying all vertices.
	_vertices.insert(_vertices.end(), mesh.positions.begin(), mesh.positions.end());
	
	const size_t trianglesCount = mesh.indices.size()/3;
	for(size_t tid = 0; tid < trianglesCount; ++tid){
		const size_t localId = 3 * tid;
		TriangleInfos triInfos;
		triInfos.v0 = indexOffset + mesh.indices[localId + 0];
		triInfos.v1 = indexOffset + mesh.indices[localId + 1];
		triInfos.v2 = indexOffset + mesh.indices[localId + 2];
		triInfos.localId = localId;
		triInfos.meshId = _meshCount;
		_triangles.push_back(triInfos);
	}
	
	Log::Info() << "[Raycaster]" << " Mesh " << _meshCount << " added, " << trianglesCount << " triangles, " << _vertices.size() - indexOffset << " vertices." << std::endl;
	
	++_meshCount;
}

void Raycaster::updateHierarchy(){
	Log::Info() << "Building hierarchy... " << std::flush;
	updateSubHierarchy(0, _triangles.size());
	Log::Info() << "Done." << std::endl;
}

int Raycaster::updateSubHierarchy(const int begin, const int count){
	// Pick a random axis for sorting.
	const int axis = Random::Int(0, 2);
	// Sort all triangles along the picked axis.
	std::sort(_triangles.begin()+begin, _triangles.begin()+begin+count, [this, axis](const TriangleInfos & t0, const TriangleInfos & t1){
		// Compute both bounding boxes.
		BoundingBox b0(_vertices[t0.v0], _vertices[t0.v1], _vertices[t0.v2]);
		BoundingBox b1(_vertices[t0.v0], _vertices[t0.v1], _vertices[t0.v2]);
		return b0.minis[axis] < b1.minis[axis];
	});
	// Create the node.
	_hierarchy.emplace_back();
	const int nodeId = _hierarchy.size()-1;
	Node currentNode;
	// If the triangles count is low enough, we have a leaf.
	if(count < 3){
		currentNode.leaf = true;
		currentNode.left = begin;
		currentNode.right = count;
		// Compute node bounding box as the union of each triangle box.
		const TriangleInfos & t0 = _triangles[begin];;
		currentNode.box = BoundingBox(_vertices[t0.v0], _vertices[t0.v1], _vertices[t0.v2]);
		for(int tid = 1; tid < count; ++tid){
			const TriangleInfos & t = _triangles[begin+tid];;
			const BoundingBox tbox(_vertices[t.v0], _vertices[t.v1], _vertices[t.v2]);
			currentNode.box.merge(tbox);
		}
	} else {
		currentNode.leaf = false;
		// Create the two sub-nodes.
		currentNode.left = updateSubHierarchy(begin, count/2);
		currentNode.right = updateSubHierarchy(begin+count/2, count-count/2);
		currentNode.box = BoundingBox(_hierarchy[currentNode.left].box);
		currentNode.box.merge(_hierarchy[currentNode.right].box);
	}
	_hierarchy[nodeId] = currentNode;
	return nodeId;
}

const Raycaster::RayHit Raycaster::intersects(const glm::vec3 & origin, const glm::vec3 & direction) const {
	const Ray ray(origin, direction);
	return intersects(ray, _hierarchy[0], 0.0001f, 1e8f);
}

const Raycaster::RayHit Raycaster::intersects(const Raycaster::Ray & ray, const Raycaster::Node & node, float mini, float maxi) const {
	if(!Raycaster::intersects(ray, node.box, mini, maxi)){
		return RayHit();
	}
	// If the node is a leaf, test all included triangles.
	if(node.leaf){
		RayHit finalHit;
		for(int tid = 0; tid < node.right; ++tid){
			const auto & tri = _triangles[node.left + tid];
			const RayHit hit = intersects(ray, tri, mini, maxi);
			if(hit.hit && hit.dist < finalHit.dist){
				finalHit = hit;
			}
		}
		return finalHit;
	}
	// Else, intersect both child nodes.
	const RayHit left = intersects(ray, _hierarchy[node.left], mini, maxi);
	// If left was hit, check if right is hit closer.
	if(left.hit){
		const RayHit right = intersects(ray, _hierarchy[node.right], mini, left.dist);
		return right.hit ? right : left;
	}
	// Check if right is hit.
	// Return the closest hit if it exists.
	const RayHit right = intersects(ray, _hierarchy[node.right], mini, maxi);
	return right;
}

const Raycaster::RayHit Raycaster::intersects(const Raycaster::Ray & ray, const TriangleInfos & tri, float mini, float maxi) const {
	// Implement Moller-Trumbore intersection test.
	const glm::vec3 & v0 = _vertices[tri.v0];
	const glm::vec3 v01 = _vertices[tri.v1] - v0;
	const glm::vec3 v02 = _vertices[tri.v2] - v0;
	const glm::vec3 p = glm::cross(ray.dir, v02);
	const float det = glm::dot(v01, p);
	
	if(std::abs(det) < 0.00001f){
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
