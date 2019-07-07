#pragma once
#include "Common.hpp"
#include "resources/MeshUtilities.hpp"

/**
 \brief Allows to cast rays against a polygonal mesh, on the CPU. Relies on an internal acceleration structure to speed up intersection queries.
 \ingroup Engine
 */
class Raycaster {
public:
	
	/** Default constructor. */
	Raycaster();

	/** Adds a mesh to the internal geometry.
	 \param mesh the mesh to add
	 \param model the transformation matrix to apply to the vertices
	 */
	void addMesh(const Mesh & mesh, const glm::mat4 & model);
	
	/** Update the internal bounding volume hierarchy.
	 \note This operation can be costful in time.
	 */
	void updateHierarchy();
	
	/** Represent a hit event between a ray and the geometry. */
	struct RayHit {
		bool hit; ///< Denote if there has been a hit.
		float dist; ///< Distance from the ray origin to the hit location.
		float u; ///< First barycentric coordinate.
		float v; ///< Second barycentric coordinate.
		float w; ///< Third barycentric coordinate.
		unsigned long localId; ///< Position of the hit triangle first vertex in the mesh index buffer.
		unsigned long meshId; ///< Index of the mesh hit by the ray.
		
		/** Default constructor ('no hit' case). */
		RayHit();
		
		/** Constructore ('hit' case).
		 \param distance the distance from the ray origin to the hit location
		 \param uu first barycentric coordinate
		 \param vv second barycentric coordinate
		 \param lid position of the hit triangle first vertex in the mesh index buffer.
		 \param mid index of the mesh hit by the ray.
		 */
		RayHit(float distance, float uu, float vv, unsigned long lid, unsigned long mid);
		
	};
	
	/** Find the closest intersection of a ray with the geometry.
	 \param origin ray origin
	 \param direction ray direction (not necessarily normalized)
	 \return a hit object containg the potential hit informations
	 */
	const RayHit intersects(const glm::vec3 & origin, const glm::vec3 & direction) const;
	
	/** Intersect a ray with the geometry.
	 \param origin ray origin
	 \param direction ray direction (not necessarily normalized)
	 \return true if the two points are joined by a free-space segment
	 */
	bool intersectsAny(const glm::vec3 & origin, const glm::vec3 & direction) const;
	
	/** Test visibility between two points.
	 \param p0 first point
	 \param p1 second point
	 \return true if the two points are joined by a free-space segment
	 \note A ray is shot from the first to the second point to test for visibility.
	 */
	bool visible(const glm::vec3 & p0, const glm::vec3 & p1) const;
	
	/** Return the interpolated position of the ray hit on the surface of the mesh.
	 \param hit the intersection record
	 \param geometry the mesh geometry information
	 \return the smooth position
	 */
	static glm::vec3 interpolatePosition(const RayHit & hit, const Mesh & geometry);
	
	/** Return the interpolated normal at the hit on the surface of the mesh.
	 \param hit the intersection record
	 \param geometry the mesh geometry information
	 \return the smooth normal (normalized)
	 */
	static glm::vec3 interpolateNormal(const RayHit & hit, const Mesh & geometry);
	
	/** Return the interpolated texture coordinates at the hit on the surface of the mesh.
	 \param hit the intersection record
	 \param geometry the mesh geometry information
	 \return the smooth texture coordinates
	 */
	static glm::vec2 interpolateUV(const RayHit & hit, const Mesh & geometry);
	
private:
	
	/** Internal triangle representation. */
	struct TriangleInfos {
		unsigned long v0; ///< First vertex index.
		unsigned long v1; ///< Second vertex index.
		unsigned long v2; ///< Third vertex index.
		unsigned long localId; ///< Position of the triangle first vertex in the mesh initial index buffer.
		unsigned int meshId; ///< Index of the mesh this triangle belongs to.
	};
	
	/** Represent a ray, shot from a given position in a specific direction. */
	struct Ray {
		const glm::vec3 pos; ///< Ray position.
		const glm::vec3 dir; ///< Ray direction (normalized).
		
		/** Constructor.
		 \param origin the position the ray was shot from
		 \param direction the direction of the ray (will be normalized)
		 */
		Ray(const glm::vec3 & origin, const glm::vec3 & direction);
		
	};
	
	/** Base element of the acceleration structure. */
	struct Node {
		BoundingBox box; ///< Boudinng box of the contained geometry.
		size_t left; ///< Index of the left child element, or first triangle index if this is a leaf.
		size_t right; ///< Index of the right child element, or number of triangles if this is a leaf.
		bool leaf; ///< Is this a leaf in the hierarchy.
	};
	
	/** Recursively updates part of the hierarchy structure.
	 \param begin begining triangle index of the region to update
	 \param count number of triangles in the region to update
	 \return the index of the updated node
	 */
	size_t updateSubHierarchy(const size_t begin, const size_t count);
	
	/** Test a ray and triangle intersection using the Muller-Trumbore test.
	 \param ray the ray
	 \param tri the triangle infos
	 \param mini the minimum allowed distance along the ray
	 \param maxi the maximum allowed distance along the ray
	 \return a hit object containg the potential hit informations
	 */
	const RayHit intersects(const Ray & ray, const TriangleInfos & tri, float mini, float maxi) const;
	
	/** Test a ray against a node of the acceleration structure, recursively.
	 \param ray the ray
	 \param node the node to test intersections against
	 \param mini the minimum allowed distance along the ray
	 \param maxi the maximum allowed distance along the ray
	 \return a hit object containg the potential closest hit informations
	 */
	const RayHit intersects(const Raycaster::Ray & ray, const Raycaster::Node & node, float mini, float maxi) const;
	
	/** Check if there is any intersection between a ray and a node in the defined range.
	 \param ray the ray
	 \param node the node to test intersections against
	 \param mini the minimum allowed distance along the ray
	 \param maxi the maximum allowed distance along the ray
	 \return true if any intersection occured
	 */
	bool intersectsAny(const Raycaster::Ray & ray, const Raycaster::Node & node, float mini, float maxi) const;
	
	/** Test a ray and bounding box intersection.
	 \param ray the ray
	 \param box the bounding box
	 \param mini the minimum allowed distance along the ray
	 \param maxi the maximum allowed distance along the ray
	 \return a boolean denoting intersection
	 */
	static bool intersects(const Ray & ray, const BoundingBox & box, float mini, float maxi);
	
	std::vector<TriangleInfos> _triangles; ///< Merged triangles informations.
	std::vector<glm::vec3> _vertices; ///< Merged vertices.
	std::vector<Node> _hierarchy; ///< Acceleration structure.
	
	unsigned int _meshCount = 0; ///< Number of meshes stored in the raycaster.
};

