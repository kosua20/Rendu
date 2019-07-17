#pragma once
#include "Common.hpp"

/**
 \brief Represent the sphere of smallest radius containing a given object or region of space.
 \ingroup Resources
 */
struct BoundingSphere {
	
	glm::vec3 center; ///< The sphere center.
	float radius; ///< The sphere radius.
	
	/** Empty sphere constructor. */
	BoundingSphere();
	
	/** Constructor
	 \param aCenter the center of the sphere
	 \param aRadius the radius of the sphere
	 */
	BoundingSphere(const glm::vec3 & aCenter, const float aRadius);
};

/**
 \brief Represent the smallest axis-aligne box containing a given object or region of space.
 \ingroup Resources
 */
struct BoundingBox {
	
	glm::vec3 minis; ///< Lower-back-left corner of the box.
	glm::vec3 maxis; ///< Higher-top-right corner of the box.
	
	/** Empty box constructor. */
	BoundingBox();
	
	/** Triangle-based box constructor.
	 \param v0 first triangle vertex
	 \param v1 second triangle vertex
	 \param v2 third triangle vertex
	 */
	BoundingBox(const glm::vec3 & v0, const glm::vec3 & v1, const glm::vec3 & v2);
	
	/** Extends the current box by another one. The result is the bounding box of the two boxes union.
	 \param box the bounding box to include
	 */
	void merge(const BoundingBox & box);
	
	/** Query the bounding sphere of this box.
	 \return the bounding sphere
	 */
	BoundingSphere getSphere() const;
	
	/** Query the size of this box.
	 \return the size
	 */
	glm::vec3 getSize() const;
	
	/** Query the positions of the eight corners of the box, in the following order (with \p m=mini, \p M=maxi):
	 \p (m,m,m), \p (m,m,M), \p (m,M,m), \p (m,M,M), \p (M,m,m), \p (M,m,M), \p (M,M,m), \p (M,M,M)
	 \return a vector containing the box corners
	 */
	std::vector<glm::vec3> getCorners() const;
	
	/** Compute the bounding box of the transformed current box.
	 \param trans the transformation to apply
	 \return the bounding box of the transformed box
	 */
	BoundingBox transformed(const glm::mat4 & trans) const;
	
	/** Indicates if a point is inside the bounding box.
	 \param point the point to check
	 \return true if the bounding box contains the point
	 */
	bool contains(const glm::vec3 & point) const;
};

/**
 \brief Represents a geometric mesh composed of vertices and triangles. For now, material information and elements/groups are not represented.
 \ingroup Resources
*/
struct Mesh {
	std::vector<glm::vec3> positions; ///< The 3D positions.
	std::vector<glm::vec3> normals; ///< The surface normals.
	std::vector<glm::vec3> tangents; ///< The surface tangents.
	std::vector<glm::vec3> binormals;  ///< The surface binormals.
	std::vector<glm::vec3> colors;  ///< The vertex colors.
	std::vector<glm::vec2> texcoords;  ///< The texture coordinates.
	std::vector<unsigned int> indices; ///< The triangular faces indices.
	
	/** Clear all data. */
	void clear();
	
};


/**
 \brief Provides utilities to load and process geometric meshes.
 \ingroup Resources
 */
class MeshUtilities {

public:
	
	/// \brief The mesh loading preprocessing mode.
	enum LoadMode {
		Expanded, ///< Duplicate vertices for every face.
		Points, ///< Load the vertices without any connectivity
		Indexed ///< Duplicate only vertices that are shared between faces with attributes with different values.
	};

	/** Load an .obj file from disk into a mesh structure.
	 \param in the input string stream from which the geometry will be loaded
	 \param mesh will be populated with the loaded geometry
	 \param mode the preprocessing mode
	 */
	static void loadObj(std::istream & in, Mesh & mesh, LoadMode mode);
	
	/** Compute the axi-aligned bounding box of a mesh.
	 \param mesh the mesh
	 \return the bounding box
	 */
	static BoundingBox computeBoundingBox(Mesh & mesh);
	
	/** Center a mesh and scale it to fit in a sphere of radius 1.0.
	 \param mesh the mesh to process
	 */
	static void centerAndUnitMesh(Mesh & mesh);
	
	/** Compute per-vertex normals based on the faces orientation.
	 \param mesh the mesh to process
	 */
	static void computeNormals(Mesh & mesh);
	
	/** Compute the tangent and binormal vectors for each vertex of a mesh.
	 \param mesh the mesh to process
	 */
	static void computeTangentsAndBinormals(Mesh & mesh);
	
	/** Save an OBJ mesh on disk.
	 \param path the path to the mesh
	 \param mesh the mesh data
	 \param defaultUVs if the mesh has no UVs, should default ones be used.
	 \return a success/error flag
	 */
	static int saveObj(const std::string & path, const Mesh & mesh, bool defaultUVs);
	
};
