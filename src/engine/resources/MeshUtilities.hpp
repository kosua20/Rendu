#ifndef MeshUtilities_h
#define MeshUtilities_h

#include "../Common.hpp"

/**
 \brief Represent the sphere of smallest radius containing a given object or region of space.
 \ingroup Resources
 */
struct BoundingSphere {
	glm::vec3 center; ///< The sphere center.
	float radius; ///< The sphere radius.
	
	/** Empty sphere constructor. */
	BoundingSphere(){
		center = glm::vec3(0.0f);
		radius = 0.0f;
	}
	
	/** Constructor
	 \param aCenter the center of the sphere
	 \param aRadius the radius of the sphere
	 */
	BoundingSphere(const glm::vec3 & aCenter, const float aRadius){
		center = aCenter; radius = aRadius;
	}
};

/**
 \brief Represent the smallest axis-aligne box containing a given object or region of space.
 \ingroup Resources
 */
struct BoundingBox {
	glm::vec3 minis; ///< Lower-back-left corner of the box.
	glm::vec3 maxis; ///< Higher-top-right corner of the box.
	
	/** Empty box constructor. */
	BoundingBox(){
		minis = glm::vec3(0.0f);
		maxis = glm::vec3(0.0f);
	}
	
	/** Extends the current box by another one. The result is the bounding box of the two boxes union.
	 \param box the bounding box to include
	 */
	void merge(const BoundingBox & box){
		minis = glm::min(minis, box.minis);
		maxis = glm::max(maxis, box.maxis);
	}
	
	/** Query the bounding sphere of this box.
	 \return the bounding sphere
	 */
	BoundingSphere getSphere() const {
		const glm::vec3 center = 0.5f*(minis+maxis);
		const float radius = glm::length(maxis-center);
		return {center, radius};
	}
	
	/** Query the positions of the eight corners of the box.
	 \return a vector containing the box corners
	 */
	std::vector<glm::vec3> getCorners() const {
		return {
			glm::vec3(minis[0], minis[1], minis[2]),
			glm::vec3(minis[0], minis[1], maxis[2]),
			glm::vec3(minis[0], maxis[1], minis[2]),
			glm::vec3(minis[0], maxis[1], maxis[2]),
			glm::vec3(maxis[0], minis[1], minis[2]),
			glm::vec3(maxis[0], minis[1], maxis[2]),
			glm::vec3(maxis[0], maxis[1], minis[2]),
			glm::vec3(maxis[0], maxis[1], maxis[2])
		};
	}
	
	/** Compute the bounding box of the transformed current box.
	 \param trans the transformation to apply
	 \return the bounding box of the transformed box
	 */
	BoundingBox transformed(const glm::mat4 & trans) const {
		BoundingBox newBox;
		const std::vector<glm::vec3> corners = getCorners();
		newBox.minis = glm::vec3(trans * glm::vec4(corners[0],1.0f));
		newBox.maxis = newBox.minis;
		for(size_t i = 0; i < 8; ++i){
			const glm::vec3 transformedCorner = glm::vec3(trans * glm::vec4(corners[i],1.0f));
			newBox.minis = glm::min(newBox.minis, transformedCorner);
			newBox.maxis = glm::max(newBox.maxis, transformedCorner);
		}
		return newBox;
	}
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
	std::vector<glm::vec2> texcoords;  ///< The texture coordinates.
	std::vector<unsigned int> indices; ///< The triangular faces indices.
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

	/** Compute the tangent and binormal vectors for each vertex of a mesh.
	 \param mesh the mesh to process
	 */
	static void computeTangentsAndBinormals(Mesh & mesh);
	
};

#endif 
