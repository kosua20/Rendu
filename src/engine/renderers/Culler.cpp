#include "renderers/Culler.hpp"
#include "resources/Bounds.hpp"

Culler::Culler(const std::vector<Object> & objects) : _objects(objects), _frustum(glm::mat4(1.0f)) {
	_order.resize(objects.size(), -1);
	_distances.resize(objects.size());
	_maxCount = (unsigned long)(objects.size());
}

const std::vector<long> & Culler::cull(const glm::mat4 & view, const glm::mat4 & proj){
	// Handle scene changes.
	const size_t objCount = _objects.size();
	if(_order.size() != objCount){
		_order.resize(objCount, -1);
	}

	// Only update frustum if not frozen in GUI.
	if(!_freezeFrustum){
		_frustum = Frustum(proj * view);
	}


	// Culling, looking only at the first maxCount objects at most.
	size_t cid = 0;
	const size_t allowedCount = std::min(objCount, size_t(_maxCount));
	for(size_t oid = 0; oid < allowedCount; ++oid){
		// If the object falls inside the frustum, store its index in the result list.
		if(_frustum.intersects(_objects[oid].boundingBox())){
			_order[cid] = long(oid);
			++cid;
		}
	}
	// Fill the rest of the result with -1s.
	for(size_t ocid = cid; ocid < objCount; ++ocid){
		_order[ocid] = -1;
	}
	return _order;
}

const std::vector<long> & Culler::cullAndSort(const glm::mat4 & view, const glm::mat4 & proj, const glm::vec3 & pos){
	// Handle scene changes.
	const size_t objCount = _objects.size();
	if(_order.size() != objCount){
		_order.resize(objCount, -1);
	}
	if(_distances.size() != objCount){
		_distances.resize(objCount);
	}

	// Only update frustum if not frozen in GUI.
	if(!_freezeFrustum){
		_frustum = Frustum(proj * view);
	}

	// Culling and distance computation.
	size_t cid = 0;
	for(size_t oid = 0; oid < objCount; ++oid){
		// If the object falls inside the frustum, compute its distance.
		const BoundingBox & bbox = _objects[oid].boundingBox();
		if(_frustum.intersects(bbox)){
			_distances[cid].id = long(oid);
			// For now use euclidean distance.
			const glm::vec3 dist = pos - bbox.getCentroid();
			_distances[cid].distance = glm::dot(dist, dist);

			++cid;
		}
	}
	// Sort wrt distances.
	std::sort(_distances.begin(), _distances.begin() + cid, [](const DistPair & a, const DistPair & b){
		return a.distance < b.distance;
	});

	// Select the first maxCount visible objects at most,
	// storing their indices in order in the result list.
	const size_t allowedCount = std::min(cid, size_t(_maxCount));
	for(size_t ocid = 0; ocid < allowedCount; ++ocid){
		_order[ocid] = _distances[ocid].id;
	}
	// Fill the rest of the result with -1s.
	for(size_t ocid = allowedCount; ocid < objCount; ++ocid){
		_order[ocid] = -1;
	}
	
	return _order;
}

void Culler::interface(){
	ImGui::Checkbox("Freeze culling", &_freezeFrustum);
	ImGui::SameLine();
	// Custom ImGui input for a ulong.
	const unsigned long step = 1, stepFast = 100;
	ImGui::InputScalar("Max objects", ImGuiDataType_U64, (void*)&_maxCount, (void*)(&step), (void*)(&stepFast), "%u", 0);

}
