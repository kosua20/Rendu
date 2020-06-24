#pragma once
#include "resources/Bounds.hpp"
#include "graphics/GPUObjects.hpp"
#include "Common.hpp"

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

	/** Default constructor.
	 \param name the mesh identifier
	 */
	Mesh(const std::string & name);
	
	/** Load an .obj file from disk into a mesh structure.
	 \param in the input string stream from which the geometry will be loaded
	 \param mode the preprocessing mode
	 \param name the mesh identifier
	 */
	Mesh(std::istream & in, Load mode, const std::string & name);

	
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

	/** Center a mesh and scale it to fit in a sphere of radius 1.0.
	 */
	void centerAndUnit();

	/** Compute per-vertex normals based on the faces orientation.
	 */
	void computeNormals();

	/** Compute the tangent and binormal vectors for each vertex of a mesh.
	 \param force Compute local tangent frame even if texture coordinates are not available.
	 */
	void computeTangentsAndBinormals(bool force);

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
	
	/** Copy assignment operator (disabled).
	 \return a reference to the object assigned to
	 */
	Mesh & operator=(const Mesh &) = delete;
	
	/** Copy constructor (disabled). */
	Mesh(const Mesh &) = delete;
	
	/** Move assignment operator .
	 \return a reference to the object assigned to
	 */
	Mesh & operator=(Mesh &&) = default;
	
	/** Move constructor. */
	Mesh(Mesh &&) = default;
	
	std::vector<glm::vec3> positions;  ///< The 3D positions.
	std::vector<glm::vec3> normals;	///< The surface normals.
	std::vector<glm::vec3> tangents;   ///< The surface tangents.
	std::vector<glm::vec3> binormals;  ///< The surface binormals.
	std::vector<glm::vec3> colors;	 ///< The vertex colors.
	std::vector<glm::vec2> texcoords;  ///< The texture coordinates.
	std::vector<unsigned int> indices; ///< The triangular faces indices.
	
	BoundingBox bbox;			  ///< The mesh bounding box in model space.
	std::unique_ptr<GPUMesh> gpu; ///< The GPU buffers infos (optional).

private:
	
	std::string _name; ///< Resource name.
};
