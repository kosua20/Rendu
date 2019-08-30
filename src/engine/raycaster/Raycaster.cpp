#include "raycaster/Raycaster.hpp"
#include "system/Random.hpp"
#include <queue>
#include <stack>

Raycaster::Ray::Ray(const glm::vec3 & origin, const glm::vec3 & direction) :
	pos(origin), dir(glm::normalize(direction)), invdir(1.0f / glm::normalize(direction)) {
}

Raycaster::RayHit::RayHit() :
	hit(false), dist(std::numeric_limits<float>::max()), u(0.0f), v(0.0f), w(0.0f), localId(0), meshId(0), internalId(0) {
}

Raycaster::RayHit::RayHit(float distance, float uu, float vv, unsigned long lid, unsigned long mid) :
	hit(true), dist(distance), u(uu), v(vv), w(1.0f - uu - vv), localId(lid), meshId(mid), internalId(0) {
}

void Raycaster::addMesh(const Mesh & mesh, const glm::mat4 & model) {
	const unsigned long indexOffset = static_cast<unsigned long>(_vertices.size());

	// Start by copying all vertices.
	const size_t startIndex = _vertices.size();
	_vertices.insert(_vertices.end(), mesh.positions.begin(), mesh.positions.end());
	const size_t endIndex = _vertices.size();

	if(model != glm::mat4(1.0f)) {
		for(size_t vid = startIndex; vid < endIndex; ++vid) {
			_vertices[vid] = glm::vec3(model * glm::vec4(_vertices[vid], 1.0f));
		}
	}

	const size_t startTriangle  = _triangles.size();
	const size_t trianglesCount = mesh.indices.size() / 3;
	for(size_t tid = 0; tid < trianglesCount; ++tid) {
		const size_t localId = 3 * tid;
		TriangleInfos triInfos;
		triInfos.v0		 = indexOffset + mesh.indices[localId + 0];
		triInfos.v1		 = indexOffset + mesh.indices[localId + 1];
		triInfos.v2		 = indexOffset + mesh.indices[localId + 2];
		triInfos.localId = static_cast<unsigned long>(localId);
		triInfos.meshId  = _meshCount;
		triInfos.box	 = BoundingBox(_vertices[triInfos.v0], _vertices[triInfos.v1], _vertices[triInfos.v2]);
		_triangles.push_back(triInfos);
	}

	// Add initial node to the hierarchy.
	_hierarchy.emplace_back();
	Node & node = _hierarchy.back();
	node.left   = startTriangle;
	node.right  = trianglesCount;

	Log::Info() << "[Raycaster]"
				<< " Mesh " << _meshCount << " added, " << trianglesCount << " triangles, " << _vertices.size() - indexOffset << " vertices." << std::endl;

	++_meshCount;
}

void Raycaster::updateHierarchy() {

	Log::Info() << "[Raycaster] Building hierarchy for " << _triangles.size() << " triangles... " << std::flush;

	struct SetInfos {
		size_t id;
		size_t begin;
		size_t count;
	};

	std::stack<SetInfos> remainingSets;
	// One root node per mesh.
	for(size_t nid = 0; nid < _meshCount; ++nid) {
		const Node & node = _hierarchy[nid];
		remainingSets.push({nid, node.left, node.right});
	}

	while(!remainingSets.empty()) {
		// Get the next node to process on the stack.
		const SetInfos current(remainingSets.top());
		remainingSets.pop();
		const size_t begin = current.begin;
		const size_t count = current.count;

		// Compute the global bounding box.
		BoundingBox global(_triangles[begin].box);
		for(size_t tid = 1; tid < count; ++tid) {
			global.merge(_triangles[begin + tid].box);
		}

		Node & node = _hierarchy[current.id];
		node.box	= global;

		// If the triangles count is low enough, we have a leaf.
		if(count < 3) {
			node.leaf  = true;
			node.left  = begin;
			node.right = count;

		} else {
			node.leaf		  = false;
			size_t splitCount = 0;
			// Pick the dimension along which the global bounding box is the largest.
			const glm::vec3 boxSize = global.getSize();
			const int axis			= (boxSize.x >= boxSize.y && boxSize.x >= boxSize.z) ? 0 : (boxSize.y >= boxSize.z ? 1 : 2);

			if(count >= 5) {

				// Compute the midpoint of all triangles centroids along the picked axis.
				float abscisse = 0.0f;
				for(size_t tid = 0; tid < count; ++tid) {
					abscisse += _triangles[begin + tid].box.getCentroid()[axis];
				}
				abscisse /= float(count);

				// Split in two subnodes.
				// Main criterion: split at the midpoint along the chosen axis.
				const auto split = std::partition(_triangles.begin() + begin, _triangles.begin() + begin + count, [abscisse, axis](const TriangleInfos & t0) {
					return t0.box.getCentroid()[axis] < abscisse;
				});
				splitCount		 = std::distance(_triangles.begin() + begin, split);
			}

			// Fallback criterion: split in two equal size subsets.
			// This can happen in case the primitive boxes overlap a lot,
			// or in case of equal coordinates along the chosen axis.
			if(splitCount == 0 || splitCount == count || count < 5) {
				splitCount = count / 2;
				std::nth_element(_triangles.begin() + begin, _triangles.begin() + begin + splitCount, _triangles.begin() + begin + count, [axis](const TriangleInfos & t0, const TriangleInfos & t1) {
					return t0.box.getCentroid()[axis] < t1.box.getCentroid()[axis];
				});
			}

			// Create the left and right sub-nodes.
			// Can't use the currentNode reference anymore because of emplace_back.
			// Left:
			_hierarchy.emplace_back();
			const size_t leftPos		= _hierarchy.size() - 1;
			_hierarchy[current.id].left = leftPos;
			remainingSets.push({leftPos, begin, splitCount});
			// Right:
			_hierarchy.emplace_back();
			const size_t rightPos		 = _hierarchy.size() - 1;
			_hierarchy[current.id].right = rightPos;
			remainingSets.push({rightPos, begin + splitCount, count - splitCount});
		}
	}
	Log::Info() << "Done: " << _hierarchy.size() << " nodes created." << std::endl;
}

Raycaster::RayHit Raycaster::intersects(const glm::vec3 & origin, const glm::vec3 & direction, float mini, float maxi) const {
	const Ray ray(origin, direction);

	std::stack<size_t> nodesToTest;
	// Start by testing each object.
	for(size_t nid = 0; nid < _meshCount; ++nid) {
		// If the ray doesn't intersect the bounding box, move to the next node.
		if(!Raycaster::intersects(ray, _hierarchy[nid].box, mini, maxi)) {
			continue;
		}
		nodesToTest.push(nid);
	}

	RayHit bestHit;
	while(!nodesToTest.empty()) {
		const Node & node = _hierarchy[nodesToTest.top()];
		nodesToTest.pop();

		// If the node is a leaf, test all included triangles.
		if(node.leaf) {
			for(size_t tid = 0; tid < node.right; ++tid) {
				const auto & tri = _triangles[node.left + tid];
				const RayHit hit = intersects(ray, tri, mini, maxi);
				// We found a valid hit.
				if(hit.hit && hit.dist < bestHit.dist) {
					bestHit = hit;
					maxi	= bestHit.dist;
				}
			}
			// Move to the next node.
			continue;
		}
		// Else, intersect both child nodes.
		if(Raycaster::intersects(ray, _hierarchy[node.left].box, mini, maxi)) {
			nodesToTest.push(node.left);
		}
		if(Raycaster::intersects(ray, _hierarchy[node.right].box, mini, maxi)) {
			nodesToTest.push(node.right);
		}
	}
	return bestHit;
}

bool Raycaster::intersectsAny(const glm::vec3 & origin, const glm::vec3 & direction, float mini, float maxi) const {
	const Ray ray(origin, direction);

	std::stack<size_t> nodesToTest;
	// Start by testing each object.
	for(size_t nid = 0; nid < _meshCount; ++nid) {
		// If the ray doesn't intersect the bounding box, move to the next node.
		if(!Raycaster::intersects(ray, _hierarchy[nid].box, mini, maxi)) {
			continue;
		}
		nodesToTest.push(nid);
	}

	while(!nodesToTest.empty()) {
		const Node & node = _hierarchy[nodesToTest.top()];
		nodesToTest.pop();

		// If the node is a leaf, test all included triangles.
		if(node.leaf) {
			for(size_t tid = 0; tid < node.right; ++tid) {
				const auto & tri = _triangles[node.left + tid];
				if(intersects(ray, tri, mini, maxi).hit) {
					return true;
				}
			}
			// No intersection move to the next node.
			continue;
		}
		// Check if any of the children is hit.
		if(Raycaster::intersects(ray, _hierarchy[node.left].box, mini, maxi)) {
			nodesToTest.push(node.left);
		}
		if(Raycaster::intersects(ray, _hierarchy[node.right].box, mini, maxi)) {
			nodesToTest.push(node.right);
		}
	}
	return false;
}

bool Raycaster::visible(const glm::vec3 & p0, const glm::vec3 & p1) const {
	const glm::vec3 direction = p1 - p0;
	const float maxi		  = glm::length(direction);
	return !intersectsAny(p0, direction, 0.0001f, maxi);
}

Raycaster::RayHit Raycaster::intersects(const Raycaster::Ray & ray, const TriangleInfos & tri, float mini, float maxi) const {
	// Implement Moller-Trumbore intersection test.
	const glm::vec3 & v0 = _vertices[tri.v0];
	const glm::vec3 v01  = _vertices[tri.v1] - v0;
	const glm::vec3 v02  = _vertices[tri.v2] - v0;
	const glm::vec3 p	= glm::cross(ray.dir, v02);
	const float det		 = glm::dot(v01, p);

	if(std::abs(det) < std::numeric_limits<float>::epsilon()) {
		return {};
	}

	const float invDet = 1.0f / det;
	const glm::vec3 q  = ray.pos - v0;
	const float u	  = invDet * glm::dot(q, p);
	if(u < 0.0f || u > 1.0f) {
		return {};
	}

	const glm::vec3 r = glm::cross(q, v01);
	const float v	 = invDet * glm::dot(ray.dir, r);
	if(v < 0.0f || (u + v) > 1.0f) {
		return {};
	}

	const float t = invDet * glm::dot(v02, r);
	if(t > mini && t < maxi) {
		return {t, u, v, tri.localId, tri.meshId};
	}
	return {};
}

bool Raycaster::intersects(const Raycaster::Ray & ray, const BoundingBox & box, float mini, float maxi) {
	const glm::vec3 minRatio = (box.minis - ray.pos) * ray.invdir;
	const glm::vec3 maxRatio = (box.maxis - ray.pos) * ray.invdir;
	const glm::vec3 minFinal = glm::min(minRatio, maxRatio);
	const glm::vec3 maxFinal = glm::max(minRatio, maxRatio);

	const float closest  = std::max(minFinal[0], std::max(minFinal[1], minFinal[2]));
	const float furthest = std::min(maxFinal[0], std::min(maxFinal[1], maxFinal[2]));

	return std::max(closest, mini) <= std::min(furthest, maxi);
}

glm::vec3 Raycaster::interpolatePosition(const RayHit & hit, const Mesh & geometry) {
	const unsigned long triId = hit.localId;
	const unsigned long i0	= geometry.indices[triId];
	const unsigned long i1	= geometry.indices[triId + 1];
	const unsigned long i2	= geometry.indices[triId + 2];
	return hit.w * geometry.positions[i0] + hit.u * geometry.positions[i1] + hit.v * geometry.positions[i2];
}

glm::vec3 Raycaster::interpolateNormal(const RayHit & hit, const Mesh & geometry) {
	const unsigned long triId = hit.localId;
	const unsigned long i0	= geometry.indices[triId];
	const unsigned long i1	= geometry.indices[triId + 1];
	const unsigned long i2	= geometry.indices[triId + 2];
	const glm::vec3 n		  = hit.w * geometry.normals[i0] + hit.u * geometry.normals[i1] + hit.v * geometry.normals[i2];
	return glm::normalize(n);
}

glm::vec2 Raycaster::interpolateUV(const RayHit & hit, const Mesh & geometry) {
	const unsigned long triId = hit.localId;
	const unsigned long i0	= geometry.indices[triId];
	const unsigned long i1	= geometry.indices[triId + 1];
	const unsigned long i2	= geometry.indices[triId + 2];
	return hit.w * geometry.texcoords[i0] + hit.u * geometry.texcoords[i1] + hit.v * geometry.texcoords[i2];
}
