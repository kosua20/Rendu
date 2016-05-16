#include "MeshUtilities.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstddef>
#include <map>

using namespace std;

void loadObj(const std::string & filename, mesh_t & mesh, LoadMode mode){
	// Open the file.
	ifstream in;
	in.open(filename.c_str());
	if (!in) {
		cerr << filename + " is not a valid file." << endl;
		return;
	}
	//Init the mesh.
	mesh.indices.clear();
	mesh.positions.clear();
	mesh.normals.clear();
	mesh.texcoords.clear();
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
		stringstream ss(res);
		vector<string> tokens;
		string token;
		while(ss >> token){
			tokens.push_back(token);
		}
		if(tokens.size() < 1){
			continue;
		}
		// Check what kind of element the line represent.
		if (tokens[0] == "v") { // Vertex position
			// We need 3 coordinates.
			if(tokens.size() < 4){
				continue;
			}
			glm::vec3 pos = glm::vec3(stof(tokens[1],NULL),stof(tokens[2],NULL),stof(tokens[3],NULL));
			positions_temp.push_back(pos);
			
		} else if (tokens[0] == "vn"){ // Vertex normal
			// We need 3 coordinates.
			if(tokens.size() < 4){
				continue;
			}
			glm::vec3 nor = glm::vec3(stof(tokens[1],NULL),stof(tokens[2],NULL),stof(tokens[3],NULL));
			normals_temp.push_back(nor);
			
		} else if (tokens[0] == "vt") { // Vertex UV
			// We need 2 coordinates.
			if(tokens.size() < 3){
				continue;
			}
			glm::vec2 uv = glm::vec2(stof(tokens[1],NULL),stof(tokens[2],NULL));
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
	in.close();
}

void centerAndUnitMesh(mesh_t & mesh){

}
