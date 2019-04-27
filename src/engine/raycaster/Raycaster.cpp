#include "Raycaster.hpp"


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
	// \todo See if we also need to keep the normals.
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

const Raycaster::RayHit Raycaster::intersects(const glm::vec3 & origin, const glm::vec3 & direction) const {
	// For now, bruteforce check all triangles.
	Ray ray(origin, direction);
	RayHit finalHit;
	for(const auto & tri : _triangles){
		const RayHit hit = intersects(ray, tri);
		if(hit.hit && hit.dist < finalHit.dist){
			finalHit = hit;
		}
	}
	return finalHit;
}

#define EPSILON 0.00001f

const Raycaster::RayHit Raycaster::intersects(const Raycaster::Ray & ray, const TriangleInfos & tri) const {
	// Implement Moller-Trumbore intersection test.
	const glm::vec3 & v0 = _vertices[tri.v0];
	const glm::vec3 v01 = _vertices[tri.v1] - v0;
	const glm::vec3 v02 = _vertices[tri.v2] - v0;
	const glm::vec3 p = glm::cross(ray.dir, v02);
	const float det = glm::dot(v01, p);
	
	if(std::abs(det) < EPSILON){
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
	
	return RayHit(t, u, v, tri.localId, tri.meshId);
}
