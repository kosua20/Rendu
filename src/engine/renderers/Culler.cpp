#include "renderers/Culler.hpp"
#include "resources/Bounds.hpp"
#include "scene/Material.hpp"

Culler::Culler(const std::vector<Object> & objects) : _objects(objects), _frustum(glm::mat4(1.0f)) {
	_order.resize(objects.size(), -1);
	_distances.resize(objects.size());
	_maxCount = (unsigned long)(objects.size());
}

const Culler::List & Culler::cull(const glm::mat4 & view, const glm::mat4 & proj){
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

const Culler::List & Culler::cullAndSort(const glm::mat4 & view, const glm::mat4 & proj, const glm::vec3 & pos){
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

	// Predefined sorting order.
	static const std::unordered_map<Material::Type, Ordering> orders = {
		{ Material::None, 			Ordering::FRONT_TO_BACK },
		{ Material::Regular, 		Ordering::FRONT_TO_BACK },
		{ Material::Parallax, 		Ordering::FRONT_TO_BACK },
		{ Material::Clearcoat, 		Ordering::FRONT_TO_BACK },
		{ Material::Anisotropic, 	Ordering::FRONT_TO_BACK },
		{ Material::Sheen, 			Ordering::FRONT_TO_BACK },
		{ Material::Iridescent, 	Ordering::FRONT_TO_BACK },
		{ Material::Emissive, 		Ordering::FRONT_TO_BACK },
		{ Material::Transparent, 	Ordering::BACK_TO_FRONT },
	};
	static const std::unordered_map<Material::Type, long> sets = {
		{ Material::None, 			0 },
		{ Material::Regular, 		1 },
		{ Material::Parallax, 		1 },
		{ Material::Clearcoat, 		1 },
		{ Material::Anisotropic, 	1 },
		{ Material::Sheen, 			1 },
		{ Material::Iridescent, 	1 },
		{ Material::Emissive, 		1 },
		{ Material::Transparent, 	2 },
	};

	// Culling and distance computation.
	size_t cid = 0;
	for(size_t oid = 0; oid < objCount; ++oid){
		// If the object falls inside the frustum, compute its distance.
		const BoundingBox & bbox = _objects[oid].boundingBox();
		if(_frustum.intersects(bbox)){
			_distances[cid].id = long(oid);

			const Material::Type & type = _objects[oid].material().type();
			const double sign = orders.at(type) == Ordering::FRONT_TO_BACK ? 1.0 : -1.0;
			const glm::vec3 dist = pos - bbox.getCentroid();

			_distances[cid].distance = sign * double(glm::dot(dist, dist));
			_distances[cid].material = sets.at(type);

			++cid;
		}
	}
	// Sort wrt distances.
	std::sort(_distances.begin(), _distances.begin() + cid, [](const DistPair & a, const DistPair & b){
		// Prioritize materials.
		if(a.material < b.material){
			return true;
		} else if( a.material > b.material){
			return false;
		}
		// Else look at distance.
		return (a.distance < b.distance);
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
