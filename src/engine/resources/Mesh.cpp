#include "resources/Mesh.hpp"
#include "renderers/DebugViewer.hpp"
#include "graphics/GPUObjects.hpp"
#include "graphics/GPU.hpp"
#include "system/TextUtilities.hpp"

#include <sstream>
#include <fstream>
#include <cstddef>

Mesh::Mesh(const std::string & name) : _name(name) {

}

Mesh::Mesh(std::istream & in, Mesh::Load mode, const std::string & name) {
	_name = name;
	
	using namespace std;

	// Init temporary vectors.
	vector<glm::vec3> positions_temp;
	vector<glm::vec3> normals_temp;
	vector<glm::vec2> texcoords_temp;
	vector<string> faces_temp;

	string res;

	// Iterate over the lines of the file.
	while(!in.eof()) {
		getline(in, res);

		// Ignore the line if it is too short or a comment.
		if(res.empty() || res[0] == '#' || res.size() < 2) {
			continue;
		}
		//We want to split the content of the line at spaces, use a stringstream directly
		res = TextUtilities::trim(res, "\r");
		stringstream ss(res);
		vector<string> tokens;
		string token;
		while(ss >> token) {
			tokens.push_back(token);
		}
		if(tokens.empty()) {
			continue;
		}
		// Check what kind of element the line represent.
		if(tokens[0] == "v") { // Vertex position
			// We need 3 coordinates.
			if(tokens.size() < 4) {
				continue;
			}
			glm::vec3 pos = glm::vec3(stof(tokens[1], nullptr), stof(tokens[2], nullptr), stof(tokens[3], nullptr));
			positions_temp.push_back(pos);

		} else if(tokens[0] == "vn") { // Vertex normal
			// We need 3 coordinates.
			if(tokens.size() < 4) {
				continue;
			}
			glm::vec3 nor = glm::vec3(stof(tokens[1], nullptr), stof(tokens[2], nullptr), stof(tokens[3], nullptr));
			normals_temp.push_back(nor);

		} else if(tokens[0] == "vt") { // Vertex UV
			// We need 2 coordinates.
			if(tokens.size() < 3) {
				continue;
			}
			glm::vec2 uv = glm::vec2(stof(tokens[1], nullptr), 1.0f - stof(tokens[2], nullptr));
			texcoords_temp.push_back(uv);

		} else if(tokens[0] == "f") { // Face indices.
			// We need 3 elements, each containing at most three indices.
			if(tokens.size() < 4) {
				continue;
			}
			faces_temp.push_back(tokens[1]);
			faces_temp.push_back(tokens[2]);
			faces_temp.push_back(tokens[3]);
		}
		// Ignore s, l, g, matl or others.
	}

	// If no vertices, end.
	if(positions_temp.empty()) {
		return;
	}

	// Does the mesh have UV or normal coordinates ?
	bool hasUV		= !texcoords_temp.empty();
	bool hasNormals = !normals_temp.empty();

	// Depending on the chosen extraction mode, we fill the mesh arrays accordingly.
	if(mode == Mesh::Load::Points) {
		// Mode: Points
		// In this mode, we don't care about faces. We simply associate each vertex/normal/uv in the same order.

		positions = positions_temp;
		if(hasNormals) {
			normals = normals_temp;
		}
		if(hasUV) {
			texcoords = texcoords_temp;
		}

	} else if(mode == Mesh::Load::Expanded) {
		// Mode: Expanded
		// In this mode, vertices are all duplicated. Each face has its set of 3 vertices, not shared with any other face.

		// For each face, query the needed positions, normals and uvs, and add them to the mesh structure.
		for(size_t i = 0; i < faces_temp.size(); i++) {
			string str	= faces_temp[i];
			size_t foundF = str.find_first_of('/');
			size_t foundL = str.find_last_of('/');

			// Positions (we are sure they exist).
			long ind1 = stol(str.substr(0, foundF)) - 1;
			positions.push_back(positions_temp[ind1]);

			// UVs (second index).
			if(hasUV) {
				long ind2 = stol(str.substr(foundF + 1, foundL)) - 1;
				texcoords.push_back(texcoords_temp[ind2]);
			}

			// Normals (third index, in all cases).
			if(hasNormals) {
				long ind3 = stol(str.substr(foundL + 1)) - 1;
				normals.push_back(normals_temp[ind3]);
			}

			//Indices (simply a vector of increasing integers).
			indices.push_back(uint(i));
		}

	} else if(mode == Mesh::Load::Indexed) {
		// Mode: Indexed
		// In this mode, vertices are only duplicated if they were already used in a previous face with a different set of uv/normal coordinates.

		// Keep track of previously encountered (position,uv,normal).
		unordered_map<string, unsigned int> indices_used;

		//Positions
		unsigned int maxInd = 0;
		for(const string & str : faces_temp) {
			//Does the association of attributs already exists ?
			if(indices_used.count(str) > 0) {
				// Just store the index in the indices vector.
				indices.push_back(indices_used[str]);
				// Go to next face.
				continue;
			}

			// else, query the associated position/uv/normal, store it, update the indices vector and the list of used elements.
			size_t foundF = str.find_first_of('/');
			size_t foundL = str.find_last_of('/');

			//Positions (we are sure they exist)
			unsigned int ind1 = stoi(str.substr(0, foundF)) - 1;
			positions.push_back(positions_temp[ind1]);

			//UVs (second index)
			if(hasUV) {
				unsigned int ind2 = stoi(str.substr(foundF + 1, foundL)) - 1;
				texcoords.push_back(texcoords_temp[ind2]);
			}
			//Normals (third index, in all cases)
			if(hasNormals) {
				unsigned int ind3 = stoi(str.substr(foundL + 1)) - 1;
				normals.push_back(normals_temp[ind3]);
			}

			indices.push_back(maxInd);
			indices_used[str] = maxInd;
			maxInd++;
		}
		indices_used.clear();
	}

	positions_temp.clear();
	normals_temp.clear();
	texcoords_temp.clear();
	faces_temp.clear();
	Log::Verbose() << Log::Resources << "Mesh loaded with " << indices.size() / 3 << " faces, " << positions.size() << " vertices, " << normals.size() << " normals, " << texcoords.size() << " texcoords." << std::endl;

	updateMetrics();
}

void Mesh::upload() {
	GPU::setupMesh(*this);
	DebugViewer::trackDefault(this);
}

void Mesh::clearGeometry() {
	positions.clear();
	normals.clear();
	tangents.clear();
	binormals.clear();
	texcoords.clear();
	indices.clear();
	// Don't update the metrics automatically
}

void Mesh::clean() {
	clearGeometry();
	bbox = BoundingBox();
	if(gpu) {
		gpu->clean();
		DebugViewer::untrackDefault(this);
	}
	// Both CPU and GPU are reset, so we can update the metrics.
	updateMetrics();
}

Buffer& Mesh::vertexBuffer(){
	assert(gpu != nullptr);
	return *gpu->vertexBuffer;
}

Buffer& Mesh::indexBuffer(){
	assert(gpu != nullptr);
	return *gpu->indexBuffer;
}

BoundingBox Mesh::computeBoundingBox() {
	bbox = BoundingBox();
	if(positions.empty()) {
		return bbox;
	}
	bbox.minis = bbox.maxis  = positions[0];
	const size_t numVertices = positions.size();
	for(size_t vid = 1; vid < numVertices; ++vid) {
		bbox.minis = glm::min(bbox.minis, positions[vid]);
		bbox.maxis = glm::max(bbox.maxis, positions[vid]);
	}

	updateMetrics();

	return bbox;
}

void Mesh::computeNormals() {
	normals.resize(positions.size());
	for(auto & n : normals) {
		n = glm::vec3(0.0f);
	}
	// Iterate over faces.
	for(size_t tid = 0; tid < indices.size(); tid += 3) {
		const unsigned int i0 = indices[tid + 0];
		const unsigned int i1 = indices[tid + 1];
		const unsigned int i2 = indices[tid + 2];
		const glm::vec3 & v0  = positions[i0];
		const glm::vec3 & v1  = positions[i1];
		const glm::vec3 & v2  = positions[i2];
		// Compute cross product between the two edges of the triangle.
		const glm::vec3 d01	= glm::normalize(v1 - v0);
		const glm::vec3 d02	= glm::normalize(v2 - v0);
		const glm::vec3 normal = glm::cross(d01, d02);
		normals[i0] += normal;
		normals[i1] += normal;
		normals[i2] += normal;
	}
	// Average for each vertex normal.
	for(auto & n : normals) {
		n = glm::normalize(n);
	}

	updateMetrics();
}

void Mesh::computeTangentsAndBinormals(bool force) {
	const bool uvAvailable = !texcoords.empty();
	if(positions.empty() || normals.empty() || (!uvAvailable && !force)) {
		// No available info.
		return;
	}

	// Start by filling everything with 0 (as we want to accumulate tangents and binormals coming from different faces for each vertex).
	for(size_t pid = 0; pid < positions.size(); ++pid) {
		tangents.emplace_back(0.0f);
		binormals.emplace_back(0.0f);
	}
	// Then, compute both vectors for each face and accumulate them.
	for(size_t fid = 0; fid < indices.size(); fid += 3) {

		const unsigned int i0 = indices[fid + 0];
		const unsigned int i1 = indices[fid + 1];
		const unsigned int i2 = indices[fid + 2];

		// Get the vertices of the face.
		const glm::vec3 & v0 = positions[i0];
		const glm::vec3 & v1 = positions[i1];
		const glm::vec3 & v2 = positions[i2];
		const glm::vec3 deltaPosition1 = v1 - v0;
		const glm::vec3 deltaPosition2 = v2 - v0;

		bool done = false;
		glm::vec3 tangent  = glm::vec3(0.0f);
		glm::vec3 binormal = glm::vec3(0.0f);

		// Get the uvs of the face if available.
		if(uvAvailable){
			const glm::vec2 & uv0 = texcoords[i0];
			const glm::vec2 & uv1 = texcoords[i1];
			const glm::vec2 & uv2 = texcoords[i2];
			// Note: our UVs are flipped to accomodate Vulkan texture orientation.
			const glm::vec2 deltaUv1(uv1[0] - uv0[0], -uv1[1] + uv0[1]);
			const glm::vec2 deltaUv2(uv2[0] - uv0[0], -uv2[1] + uv0[1]);
			// Compute tangent and binormal for the face.
			const float denom  = deltaUv1.x * deltaUv2.y - deltaUv1.y * deltaUv2.x;
			if(std::abs(denom) >= 0.001f) {
				const float det = (1.0f / denom);
				tangent			= det * (deltaPosition1 * deltaUv2.y - deltaPosition2 * deltaUv1.y);
				binormal		= det * (deltaPosition2 * deltaUv1.x - deltaPosition1 * deltaUv2.x);
				done = true;
			}
		}
		// Fallback to basic frame.
		if(!done){
			const glm::vec3 normal = glm::normalize(glm::cross(deltaPosition1, deltaPosition2));
			tangent				   = std::abs(normal.z) > 0.8f ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 0.0f, 1.0f);
			binormal			   = glm::normalize(glm::cross(normal, tangent));
			tangent				   = glm::normalize(glm::cross(binormal, normal));
		}
		// Accumulate them. We don't normalize to get a free weighting based on the size of the face.
		tangents[i0] += tangent;
		tangents[i1] += tangent;
		tangents[i2] += tangent;

		binormals[i0] += binormal;
		binormals[i1] += binormal;
		binormals[i2] += binormal;
	}
	// Finally, enforce orthogonality and good orientation of the basis.
	for(size_t tid = 0; tid < tangents.size(); ++tid) {

		// Detect tangent cancellations and orthogonality.
		if(glm::all(glm::equal(glm::cross(tangents[tid], normals[tid]), glm::vec3(0.0f)))){
			// Assume normal is never zero.
			tangents[tid] = glm::cross(glm::normalize(binormals[tid]), normals[tid]);
		}
		// Re-update tangent.
		tangents[tid] = glm::normalize(tangents[tid] - normals[tid] * glm::dot(normals[tid], tangents[tid]));
		if(glm::dot(glm::cross(normals[tid], tangents[tid]), binormals[tid]) < 0.0f) {
			tangents[tid] *= -1.0f;
		}
		// Detect binormal cancellations.
		if(glm::length(binormals[tid]) < 0.1f){
			binormals[tid] = glm::cross(normals[tid], tangents[tid]);
		}
		binormals[tid] = glm::normalize(binormals[tid]);
	}
	Log::Verbose() << Log::Resources << "Mesh: " << tangents.size() << " tangents and binormals computed." << std::endl;

	updateMetrics();
}

int Mesh::saveAsObj(const std::string & path, bool defaultUVs) {

	std::ofstream objFile(path);
	if(!objFile.is_open()) {
		Log::Error() << "Unable to create file at path \"" << path << "\"." << std::endl;
		return 1;
	}

	// Write vertices information.
	for(const auto & v : positions) {
		objFile << "v " << v.x << " " << v.y << " " << v.z << std::endl;
	}
	for(const auto & t : texcoords) {
		objFile << "vt " << t.x << " " << (1.0f - t.y) << std::endl;
	}
	for(const auto & n : normals) {
		objFile << "vn " << n.x << " " << n.y << " " << n.z << std::endl;
	}

	const bool hasNormals   = !normals.empty();
	const bool hasTexCoords = !texcoords.empty();
	// If the mesh has no UVs, it's probably using a uniform color material. We can force all vertices to have 0.5,0.5 UVs.
	std::string defUV;
	if(!hasTexCoords && defaultUVs) {
		objFile << "vt 0.5 0.5" << std::endl;
		defUV = "1";
	}

	// Faces indices.
	for(size_t tid = 0; tid < indices.size(); tid += 3) {
		const std::string t0 = std::to_string(indices[tid + 0] + 1);
		const std::string t1 = std::to_string(indices[tid + 1] + 1);
		const std::string t2 = std::to_string(indices[tid + 2] + 1);
		objFile << "f";
		objFile << " " << t0 << "/" << (hasTexCoords ? t0 : defUV) << "/" << (hasNormals ? t0 : "");
		objFile << " " << t1 << "/" << (hasTexCoords ? t1 : defUV) << "/" << (hasNormals ? t1 : "");
		objFile << " " << t2 << "/" << (hasTexCoords ? t2 : defUV) << "/" << (hasNormals ? t2 : "");
		objFile << std::endl;
	}
	objFile.close();
	return 0;
}


const std::string & Mesh::name() const {
	return _name;
}

bool Mesh::hadNormals() const {
	return _metrics.normals != 0;
}

bool Mesh::hadTexcoords() const {
	return _metrics.texcoords != 0;
}

bool Mesh::hadColors() const {
	return _metrics.colors != 0;
}

const Mesh::Metrics & Mesh::metrics() const {
	return _metrics;
}

void Mesh::updateMetrics(){
	_metrics.vertices = positions.size();
	_metrics.normals = normals.size();
	_metrics.tangents = tangents.size();
	_metrics.binormals = binormals.size();
	_metrics.colors = colors.size();
	_metrics.texcoords = texcoords.size();
	_metrics.indices = indices.size();
}

Mesh & Mesh::operator=(Mesh &&) = default;

Mesh::Mesh(Mesh &&) = default;

Mesh::~Mesh() = default;

