#pragma once
#include "resources/Bounds.hpp"
#include "graphics/GPUObjects.hpp"
#include "Common.hpp"

/**
 \brief Represents a geometric mesh composed of vertices and triangles. For now, material information and elements/groups are not represented.
 Can store both the CPU and GPU representations. Provides utilities to load and process geometric meshes.
 \ingroup Resources
*/
class Mesh {
public:
	/** Clear CPU geometry data. */
	void clearGeometry();

	/** Cleanup all data. */
	void clean();

	/** Send to the GPU. */
	void upload();

	std::vector<glm::vec3> positions;  ///< The 3D positions.
	std::vector<glm::vec3> normals;	///< The surface normals.
	std::vector<glm::vec3> tangents;   ///< The surface tangents.
	std::vector<glm::vec3> binormals;  ///< The surface binormals.
	std::vector<glm::vec3> colors;	 ///< The vertex colors.
	std::vector<glm::vec2> texcoords;  ///< The texture coordinates.
	std::vector<unsigned int> indices; ///< The triangular faces indices.

	BoundingBox bbox;			  ///< The mesh bounding box in model space.
	std::unique_ptr<GPUMesh> gpu; ///< The GPU buffers infos (optional).

	/// \brief The mesh loading preprocessing mode.
	enum class Load {
		Expanded, ///< Duplicate vertices for every face.
		Points,   ///< Load the vertices without any connectivity
		Indexed   ///< Duplicate only vertices that are shared between faces with attributes with different values.
	};

	/** Load an .obj file from disk into a mesh structure.
	 \param in the input string stream from which the geometry will be loaded
	 \param mesh will be populated with the loaded geometry
	 \param mode the preprocessing mode
	 */
	static void loadObj(std::istream & in, Mesh & mesh, Load mode);

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
