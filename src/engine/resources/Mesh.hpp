#pragma once
#include "resources/Bounds.hpp"
#include "resources/Buffer.hpp"
#include "graphics/GPUTypes.hpp"
#include "Common.hpp"

class GPUMesh;

/**
 \brief Represents a geometric mesh composed of vertices, other attributes and triangles.
 \details For now, material information and elements/groups are not represented.
 Can store both the CPU and GPU representations. Provides utilities to load and process geometric meshes.
 \ingroup Resources
*/
class Mesh {
public:
	
	/// \brief The mesh loading preprocessing mode.
	enum class Load {
		Expanded, ///< Duplicate vertices for every face.
		Points,   ///< Load the vertices without any connectivity
		Indexed   ///< Duplicate only vertices that are shared between faces with attributes with different values.
	};

	/// \brief Information on a geometric mesh.
	struct Metrics {
		size_t vertices = 0; ///< Vertex count.
		size_t normals = 0; ///< Normal count.
		size_t tangents = 0; ///< Tangent count.
		size_t bitangents = 0; ///< Bitangent count.
		size_t colors = 0; ///< Color count.
		size_t texcoords = 0; ///< UV count.
		size_t indices = 0; ///< Index count.
	};

	/** Default constructor.
	 \param name the mesh identifier
	 */
	Mesh(const std::string & name);
	
	/** Load an .obj file from disk into a mesh structure.
	 \param in the input string stream from which the geometry will be loaded
	 \param mode the preprocessing mode
	 \param name the mesh identifier
	 */
	Mesh(std::istream & in, Mesh::Load mode, const std::string & name);

	/** Send to the GPU. */
	void upload();
	
	/** Clear CPU geometry data. */
	void clearGeometry();

	/** Cleanup all data. */
	void clean();

	/** Compute the axi-aligned bounding box of a mesh.
	  Update the internal bbox and returns it.
	 \return the bounding box
	 */
	BoundingBox computeBoundingBox();

	/** Compute per-vertex normals based on the faces orientation.
	 */
	void computeNormals();

	/** Compute the tangent and bitangents vectors for each vertex of a mesh.
	 \param force Compute local tangent frame even if texture coordinates are not available.
	 */
	void computeTangentsAndBitangents(bool force);

	/** Save an OBJ mesh on disk.
	 \param path the path to the mesh
	 \param defaultUVs if the mesh has no UVs, should default ones be used.
	 \return a success/error flag
	 */
	int saveAsObj(const std::string & path, bool defaultUVs);
	
	/** Get the resource name.
	 \return the name.
	 */
	const std::string & name() const;

	/** Did the mesh contained normals initially.
	 \return true if it did
	 */
	bool hadNormals() const;

	/** Did the mesh contained texture coordinates initially.
	 \return true if it did
	 */
	bool hadTexcoords() const;

	/** Did the mesh contained colors initially.
	 \return true if it did
	 */
	bool hadColors() const;

	/** \return the mesh current metrics (vertex count,...) */
	const Metrics & metrics() const;
	
	/** Copy assignment operator (disabled).
	 \return a reference to the object assigned to
	 */
	Mesh & operator=(const Mesh &) = delete;
	
	/** Copy constructor (disabled). */
	Mesh(const Mesh &) = delete;
	
	/** Move assignment operator .
	 \return a reference to the object assigned to
	 */
	Mesh & operator=(Mesh &&);
	
	/** Move constructor. */
	Mesh(Mesh &&);

	/** Destructor */
	~Mesh();

	/** \return a reference to the GPU vertex buffer if it exists */
	Buffer& vertexBuffer();
	/** \return a reference to the GPU vertex buffer if it exists */
	Buffer& indexBuffer();
	
	std::vector<glm::vec3> positions;  ///< The 3D positions.
	std::vector<glm::vec3> normals;	///< The surface normals.
	std::vector<glm::vec3> tangents;   ///< The surface tangents.
	std::vector<glm::vec3> bitangents;  ///< The surface bitangents.
	std::vector<glm::vec3> colors;	 ///< The vertex colors.
	std::vector<glm::vec2> texcoords;  ///< The texture coordinates.
	std::vector<unsigned int> indices; ///< The triangular faces indices.
	
	BoundingBox bbox;			  ///< The mesh bounding box in model space.
	std::unique_ptr<GPUMesh> gpu; ///< The GPU buffers infos (optional).

private:

	/** Update mesh metrics based on the current CPU content. */
	void updateMetrics();

	Metrics _metrics; ///< Resource stats.
	std::string _name; ///< Resource name.

};
