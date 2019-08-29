#include "resources/Mesh.hpp"
#include "system/TextUtilities.hpp"
#include "graphics/GPUObjects.hpp"
#include "graphics/GLUtilities.hpp"
#include <sstream>
#include <cstddef>


void Mesh::clearGeometry(){
	positions.clear();
	normals.clear();
	tangents.clear();
	binormals.clear();
	texcoords.clear();
	indices.clear();
}

void Mesh::clean(){
	clearGeometry();
	bbox = BoundingBox();
	if(gpu){
		gpu->clean();
	}
}

void Mesh::upload(){
	GLUtilities::setupBuffers(*this);
}

void Mesh::loadObj( std::istream & in, Mesh & mesh, Mesh::Load mode){
	
	using namespace std;
	
	//Init the mesh.
	mesh.indices.clear();
	mesh.positions.clear();
	mesh.normals.clear();
	mesh.texcoords.clear();
	mesh.colors.clear();
	
	// Init temporary vectors.
	vector<glm::vec3> positions_temp;
	vector<glm::vec3> normals_temp;
	vector<glm::vec2> texcoords_temp;
	vector<string> faces_temp;

	string res;

	// Iterate over the lines of the file.
	while(!in.eof()){
		getline(in,res);

		// Ignore the line if it is too short or a comment.
		if(res.empty() || res[0] == '#' || res.size()<2){
			continue;
		}
		//We want to split the content of the line at spaces, use a stringstream directly
		res = TextUtilities::trim(res, "\r");
		stringstream ss(res);
		vector<string> tokens;
		string token;
		while(ss >> token){
			tokens.push_back(token);
		}
		if(tokens.empty()){
			continue;
		}
		// Check what kind of element the line represent.
		if (tokens[0] == "v") { // Vertex position
			// We need 3 coordinates.
			if(tokens.size() < 4){
				continue;
			}
			glm::vec3 pos = glm::vec3(stof(tokens[1], nullptr),stof(tokens[2], nullptr),stof(tokens[3], nullptr));
			positions_temp.push_back(pos);
			
		} else if (tokens[0] == "vn"){ // Vertex normal
			// We need 3 coordinates.
			if(tokens.size() < 4){
				continue;
			}
			glm::vec3 nor = glm::vec3(stof(tokens[1], nullptr),stof(tokens[2], nullptr),stof(tokens[3], nullptr));
			normals_temp.push_back(nor);
			
		} else if (tokens[0] == "vt") { // Vertex UV
			// We need 2 coordinates.
			if(tokens.size() < 3){
				continue;
			}
			glm::vec2 uv = glm::vec2(stof(tokens[1], nullptr),stof(tokens[2], nullptr));
			texcoords_temp.push_back(uv);
			
		} else if (tokens[0] == "f") { // Face indices.
			// We need 3 elements, each containing at most three indices.
			if(tokens.size() < 4){
				continue;
			}
			faces_temp.push_back(tokens[1]);
			faces_temp.push_back(tokens[2]);
			faces_temp.push_back(tokens[3]);

		} else { // Ignore s, l, g, matl or others
			continue;
		}
	}

	// If no vertices, end.
	if(positions_temp.empty()){
			return;
	}

	// Does the mesh have UV or normal coordinates ?
	bool hasUV = !texcoords_temp.empty();
	bool hasNormals = !normals_temp.empty();

	// Depending on the chosen extraction mode, we fill the mesh arrays accordingly.
	if (mode == Mesh::Load::Points){
		// Mode: Points
		// In this mode, we don't care about faces. We simply associate each vertex/normal/uv in the same order.
		
		mesh.positions = positions_temp;
		if(hasNormals){
			mesh.normals = normals_temp;
		}
		if(hasUV){
			mesh.texcoords = texcoords_temp;
		}

	} else if(mode == Mesh::Load::Expanded){
		// Mode: Expanded
		// In this mode, vertices are all duplicated. Each face has its set of 3 vertices, not shared with any other face.
		
		// For each face, query the needed positions, normals and uvs, and add them to the mesh structure.
		for(size_t i = 0; i < faces_temp.size(); i++){
			string str = faces_temp[i];
			size_t foundF = str.find_first_of('/');
			size_t foundL = str.find_last_of('/');
			
			// Positions (we are sure they exist).
			long ind1 = stol(str.substr(0,foundF))-1;
			mesh.positions.push_back(positions_temp[ind1]);

			// UVs (second index).
			if(hasUV){
				long ind2 = stol(str.substr(foundF+1,foundL))-1;
				mesh.texcoords.push_back(texcoords_temp[ind2]);
			}

			// Normals (third index, in all cases).
			if(hasNormals){
				long ind3 = stol(str.substr(foundL+1))-1;
				mesh.normals.push_back(normals_temp[ind3]);
			}
			
			//Indices (simply a vector of increasing integers).
			mesh.indices.push_back(uint(i));
		}

	} else if (mode == Mesh::Load::Indexed){
		// Mode: Indexed
		// In this mode, vertices are only duplicated if they were already used in a previous face with a different set of uv/normal coordinates.
		
		// Keep track of previously encountered (position,uv,normal).
		map<string,unsigned int> indices_used;

		//Positions
		unsigned int maxInd = 0;
		for(size_t i = 0; i < faces_temp.size(); i++){
			
			string str = faces_temp[i];

			//Does the association of attributs already exists ?
			if(indices_used.count(str)>0){
				// Just store the index in the indices vector.
				mesh.indices.push_back(indices_used[str]);
				// Go to next face.
				continue;
			}

			// else, query the associated position/uv/normal, store it, update the indices vector and the list of used elements.
			size_t foundF = str.find_first_of('/');
			size_t foundL = str.find_last_of('/');
			
			//Positions (we are sure they exist)
			unsigned int ind1 = stoi(str.substr(0,foundF))-1;
			mesh.positions.push_back(positions_temp[ind1]);

			//UVs (second index)
			if(hasUV){
				unsigned int ind2 = stoi(str.substr(foundF+1,foundL))-1;
				mesh.texcoords.push_back(texcoords_temp[ind2]);
			}
			//Normals (third index, in all cases)
			if(hasNormals){
				unsigned int ind3 = stoi(str.substr(foundL+1))-1;
				mesh.normals.push_back(normals_temp[ind3]);
			}

			mesh.indices.push_back(maxInd);
			indices_used[str] = maxInd;
			maxInd++;
		}
		indices_used.clear();
	}

	positions_temp.clear();
	normals_temp.clear();
	texcoords_temp.clear();
	faces_temp.clear();
	Log::Verbose() << Log::Resources << "Mesh loaded with " << mesh.indices.size()/3 << " faces, " << mesh.positions.size() << " vertices, " << mesh.normals.size() << " normals, " << mesh.texcoords.size() << " texcoords." << std::endl;

}

BoundingBox Mesh::computeBoundingBox(Mesh & mesh){
	BoundingBox bbox;
	if(mesh.positions.empty()){
		return bbox;
	}
	bbox.minis = bbox.maxis = mesh.positions[0];
	const size_t numVertices = mesh.positions.size();
	for(size_t vid = 1; vid < numVertices; ++vid){
		bbox.minis = glm::min(bbox.minis, mesh.positions[vid]);
		bbox.maxis = glm::max(bbox.maxis, mesh.positions[vid]);
	}
	return bbox;
}

void Mesh::centerAndUnitMesh(Mesh & mesh){
	// Compute the centroid.
	glm::vec3 centroid = glm::vec3(0.0);
	float maxi = mesh.positions[0].x;
	for(size_t i = 0; i < mesh.positions.size(); i++){
		centroid += mesh.positions[i];
	}
	centroid /= mesh.positions.size();

	for(size_t i = 0; i < mesh.positions.size(); i++){
		// Translate  the vertex.
		mesh.positions[i] -= centroid;
		// Find the maximal distance from a vertex to the center.
		maxi = abs(mesh.positions[i].x) > maxi ? abs(mesh.positions[i].x) : maxi;
		maxi = abs(mesh.positions[i].y) > maxi ? abs(mesh.positions[i].y) : maxi;
		maxi = abs(mesh.positions[i].z) > maxi ? abs(mesh.positions[i].z) : maxi;
	}
	maxi = maxi == 0.0f ? 1.0f : maxi;
	
	// Scale the mesh.
	for(size_t i = 0; i < mesh.positions.size(); i++){
		mesh.positions[i] /= maxi;
	}
}

void Mesh::computeNormals(Mesh & mesh){
	mesh.normals.resize(mesh.positions.size());
	for(size_t pid = 0; pid < mesh.normals.size(); ++pid){
		mesh.normals[pid] = glm::vec3(0.0f);
	}
	// Iterate over faces.
	for(size_t tid = 0; tid < mesh.indices.size(); tid += 3){
		const unsigned int i0 = mesh.indices[tid + 0];
		const unsigned int i1 = mesh.indices[tid + 1];
		const unsigned int i2 = mesh.indices[tid + 2];
		const glm::vec3 & v0 = mesh.positions[i0];
		const glm::vec3 & v1 = mesh.positions[i1];
		const glm::vec3 & v2 = mesh.positions[i2];
		// Compute cross product between the two edges of the triangle.
		const glm::vec3 d01 = glm::normalize(v1 - v0);
		const glm::vec3 d02 = glm::normalize(v2 - v0);
		const glm::vec3 normal = glm::cross(d01, d02);
		mesh.normals[i0] += normal;
		mesh.normals[i1] += normal;
		mesh.normals[i2] += normal;
	}
	// Average for each vertex normal.
	for(size_t pid = 0; pid < mesh.normals.size(); ++pid){
		mesh.normals[pid] = glm::normalize(mesh.normals[pid]);
	}
}

void Mesh::computeTangentsAndBinormals(Mesh & mesh){
	if(mesh.indices.size() * mesh.positions.size() * mesh.texcoords.size() == 0){
		// Missing data, or not the right mode (Points).
		return;
	}
	// Start by filling everything with 0 (as we want to accumulate tangents and binormals coming from different faces for each vertex).
	for(size_t pid = 0; pid < mesh.positions.size(); ++pid){
		mesh.tangents.emplace_back(0.0f);
		mesh.binormals.emplace_back(0.0f);
	}
	// Then, compute both vectors for each face and accumulate them.
	for(size_t fid = 0; fid < mesh.indices.size(); fid += 3){

		const unsigned int i0 = mesh.indices[fid+0];
		const unsigned int i1 = mesh.indices[fid+1];
		const unsigned int i2 = mesh.indices[fid+2];
		
		// Get the vertices of the face.
		const glm::vec3 & v0 = mesh.positions[i0];
		const glm::vec3 & v1 = mesh.positions[i1];
		const glm::vec3 & v2 = mesh.positions[i2];
		// Get the uvs of the face.
		const glm::vec2 & uv0 = mesh.texcoords[i0];
		const glm::vec2 & uv1 = mesh.texcoords[i1];
		const glm::vec2 & uv2 = mesh.texcoords[i2];

		// Delta positions and uvs.
		const glm::vec3 deltaPosition1 = v1 - v0;
		const glm::vec3 deltaPosition2 = v2 - v0;
		const glm::vec2 deltaUv1 = uv1 - uv0;
		const glm::vec2 deltaUv2 = uv2 - uv0;

		// Compute tangent and binormal for the face.
		const float denom = deltaUv1.x * deltaUv2.y - deltaUv1.y * deltaUv2.x;
		const bool degen = std::abs(denom) < 0.001f;
		glm::vec3 tangent  = glm::vec3(0.0f);
		glm::vec3 binormal = glm::vec3(0.0f);
		// Avoid divide-by-zero if same UVs.
		if(!degen){
			const float det = (1.0f / denom);
    		tangent = det * (deltaPosition1 * deltaUv2.y - deltaPosition2 * deltaUv1.y);
    		binormal = det * (deltaPosition2 * deltaUv1.x - deltaPosition1 * deltaUv2.x);
		} else {
			const glm::vec3 normal = glm::normalize(glm::cross(deltaPosition1, deltaPosition2));
			tangent = std::abs(normal.z) > 0.8f ? glm::vec3(1.0f,0.0f,0.0f) : glm::vec3(0.0f,0.0f,1.0f);
			binormal = glm::normalize(glm::cross(normal, tangent));
			tangent = glm::normalize(glm::cross(binormal, normal));
		}
    	// Accumulate them. We don't normalize to get a free weighting based on the size of the face.
    	mesh.tangents[i0] += tangent;
    	mesh.tangents[i1] += tangent;
    	mesh.tangents[i2] += tangent;

    	mesh.binormals[i0] += binormal;
    	mesh.binormals[i1] += binormal;
    	mesh.binormals[i2] += binormal;
	}
	// Finally, enforce orthogonality and good orientation of the basis.
	for(size_t tid = 0; tid < mesh.tangents.size(); ++tid){
		mesh.tangents[tid] = normalize(mesh.tangents[tid] - mesh.normals[tid] * dot(mesh.normals[tid], mesh.tangents[tid]));
		if(dot(cross(mesh.normals[tid], mesh.tangents[tid]), mesh.binormals[tid]) < 0.0f){
			mesh.tangents[tid] *= -1.0f;
 		}
	}
	Log::Verbose() << Log::Resources << "Mesh: " << mesh.tangents.size() << " tangents and binormals computed." << std::endl;
}

int Mesh::saveObj(const std::string & path, const Mesh & mesh, bool defaultUVs){
	
	std::ofstream objFile(path);
	if(!objFile.is_open()){
		Log::Error() << "Unable to create file at path \"" << path << "\"." << std::endl;
		return 1;
	}
	
	// Write vertices information.
	for(size_t pid = 0; pid < mesh.positions.size(); ++pid){
		const glm::vec3 & v = mesh.positions[pid];
		objFile << "v " << v.x << " " << v.y << " " << v.z << std::endl;
	}
	for(size_t pid = 0; pid < mesh.texcoords.size(); ++pid){
		const glm::vec2 & t = mesh.texcoords[pid];
		objFile << "vt " << t.x << " " << t.y << std::endl;
	}
	for(size_t pid = 0; pid < mesh.normals.size(); ++pid){
		const glm::vec3 & n = mesh.normals[pid];
		objFile << "vn " << n.x << " " << n.y << " " << n.z << std::endl;
	}
	
	const bool hasNormals = !mesh.normals.empty();
	const bool hasTexCoords = !mesh.texcoords.empty();
	// If the mesh has no UVs, it's probably using a uniform color material. We can force all vertices to have 0.5,0.5 UVs.
	std::string defUV;
	if(!hasTexCoords && defaultUVs){
		objFile << "vt 0.5 0.5" << std::endl;
		defUV = "1";
	}
	
	// Faces indices.
	for(size_t tid = 0; tid < mesh.indices.size(); tid += 3){
		const std::string t0 = std::to_string(mesh.indices[tid+0] + 1);
		const std::string t1 = std::to_string(mesh.indices[tid+1] + 1);
		const std::string t2 = std::to_string(mesh.indices[tid+2] + 1);
		objFile << "f";
		objFile << " " << t0 << "/" << (hasTexCoords ? t0 : defUV) << "/" << (hasNormals ? t0 : "");
		objFile << " " << t1 << "/" << (hasTexCoords ? t1 : defUV) << "/" << (hasNormals ? t1 : "");
		objFile << " " << t2 << "/" << (hasTexCoords ? t2 : defUV) << "/" << (hasNormals ? t2 : "");
		objFile << std::endl;
	}
	objFile.close();
	return 0;
}
