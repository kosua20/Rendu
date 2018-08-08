#ifndef MeshUtilities_h
#define MeshUtilities_h

#include "../Common.hpp"

struct BoundingSphere {
	glm::vec3 center;
	float radius;
	
	BoundingSphere(){
		center = glm::vec3(0.0f);
		radius = 0.0f;
	}
	
	BoundingSphere(const glm::vec3 & aCenter, const float aRadius){
		center = aCenter; radius = aRadius;
	}
};

struct BoundingBox {
	glm::vec3 minis;
	glm::vec3 maxis;
	
	BoundingBox(){
		minis = glm::vec3(0.0f);
		maxis = glm::vec3(0.0f);
	}
	
	void merge(const BoundingBox & box){
		minis = glm::min(minis, box.minis);
		maxis = glm::max(maxis, box.maxis);
	}
	
	BoundingSphere getSphere() const {
		const glm::vec3 center = 0.5f*(minis+maxis);
		const float radius = glm::length(maxis-center);
		return {center, radius};
	}
	
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

// A mesh will be represented by a struct. For now, material information and elements/groups are not retrieved from the .obj.
struct Mesh {
	std::vector<glm::vec3> positions;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec3> tangents;
	std::vector<glm::vec3> binormals;
	std::vector<glm::vec2> texcoords;
	std::vector<unsigned int> indices;
};



class MeshUtilities {

public:
	
	// Three load modes: load the vertices without any connectivity (Points),
	// 					 load them with all vertices duplicated for each face (Expanded),
	//					 load them after duplicating only the ones that are shared between faces with multiple attributes (Indexed).
	enum LoadMode {
		Expanded, Points, Indexed
	};

	/// Load an obj file from disk into the mesh structure.
	static void loadObj(std::istream & in, Mesh & mesh, LoadMode mode);
	
	/// Compute the bounding box of the mesh.
	static BoundingBox computeBoundingBox(Mesh & mesh);
	
	/// Center the mesh and scale it to fit in the [-1,1] box.
	static void centerAndUnitMesh(Mesh & mesh);

	/// Compute the tangents and binormal vectors for each vertex.
	static void computeTangentsAndBinormals(Mesh & mesh);
	
};

#endif 
