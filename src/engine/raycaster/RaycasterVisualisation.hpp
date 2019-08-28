#pragma once
#include "resources/Mesh.hpp"
#include "raycaster/Raycaster.hpp"
#include "Common.hpp"

/**
 \brief Helper that can generate information on a raycaster internal data: BVH visualisation, etc.
 \ingroup Raycaster
 */
class RaycasterVisualisation {
public:
	
	/** Constructor.
	 \param raycaster the raycaster about which to provide visualisation
	 */
	explicit RaycasterVisualisation(const Raycaster & raycaster);
	
	/** Generate geometry to visualize each level of the bounding volume hierarchy as a series of bounding boxes.
	 \param meshes will be filled with the geometry of each depth level
	 */
	void getAllLevels(std::vector<Mesh> & meshes) const;
	
	/** Cast a ray and generate geometry for all intersected nodes at each level of the bounding volume hierarchy as a series of bounding boxes.
	 \param origin ray origin
	 \param direction ray direction (not necessarily normalized)
	 \param meshes will be filled with the geometry of each depth level
	 \param mini the minimum distance allowed for the intersection
	 \param maxi the maximum distance allowed for the intersection
	 \return a hit object containg the potential hit informations
	 */
	Raycaster::RayHit getRayLevels(const glm::vec3 & origin, const glm::vec3 & direction, std::vector<Mesh> & meshes, float mini = 0.0001f, float maxi = 1e8f) const;
	
	/** Generate a mesh representing a ray and the intersected triangle.
	 \param rayPos the ray origin
	 \param rayDir the ray direction( will be normalized)
	 \param hit the ray hit
	 \param rayMesh will be filled with the ray and triangle geometry
	 \param defaultLength the length of the ray when no collision happened
	 */
	void getRayMesh(const glm::vec3 & rayPos, const glm::vec3 & rayDir, const Raycaster::RayHit & hit, Mesh & rayMesh, float defaultLength = 10000.0f) const;
	
private:
	
	/** Infos for displaying a given node. */
	struct DisplayNode {
		size_t node; ///< The index of the node.
		size_t depth; ///< Its depth.
	};
	
	/** Generate geometry for a subset of the bounding volume hierarchy as a series of bounding boxes.
	 \param nodes the nodes to generate geometry for
	 \param meshes will be filled with the geometry of each depth level
	 */
	void createBVHMeshes(const std::vector<DisplayNode> & nodes, std::vector<Mesh> &meshes) const;
	
	const Raycaster & _raycaster; ///< The raycaster to visualise.
	
};

