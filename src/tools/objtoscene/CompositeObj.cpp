#include "CompositeObj.hpp"
#include "system/TextUtilities.hpp"


namespace CompositeObj {

	/** \brief Contains all the geometry information from an OBJ file.
	 \ingroup ObjToScene
	 */
	class RawGeometry {
	public:

		/** Clear all data. */
		void clear(){
			positions.clear();
			normals.clear();
			texcoords.clear();
			faces.clear();
		}

		std::vector<glm::vec3> positions; ///< Positions
		std::vector<glm::vec3> normals; ///< Normals
		std::vector<glm::vec2> texcoords; ///< UVs
		std::vector<std::string> faces; ///< OBJ face strings

	};

	/** \brief Associate an object and a material with geometry in an OBJ.
	 \ingroup ObjToScene
	 */
	class ObjectMaterialUse {
	public:
		
		/** Constructor.
		 \param objName object name
		 \param matName material name
		 \param faceId the object first face position
		 */
		ObjectMaterialUse(const std::string & objName, const std::string & matName, size_t faceId):
			objectName(objName), materialName(matName), index(faceId){
		}

		std::string objectName; ///< The name of the object
		std::string materialName; ///< The name of the material
		size_t index; ///< The position of the first face of the object in the OBJ file.

	};

	/** Parse an OBJ file to extract objects geometry, material library files and object-material associations.
	 \param objFile the stream to parse
	 \param geometry will contain all the geometry stored in the file
	 \param materialsFiles will contain a list of material files referenced.
	 \param objectMatUses will contain a list of object/material associations.
	 */
	void parseMultiObj(std::istream & objFile, RawGeometry & geometry, std::vector<std::string> & materialsFiles, std::vector<ObjectMaterialUse> & objectMatUses) {
	
		using namespace std;
		
		// Iterate over the lines of the file.
		string res;
		size_t faceNumber = 0;
		while(!objFile.eof()){
			
			getline(objFile,res);
			// Ignore the line if it is too short or a comment.
			if(res.length() < 2 || res[0] == '#'){
				continue;
			}
			
			// We want to split the content of the line at spaces, use a stringstream directly
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
			if (tokens[0] == "v" && tokens.size() >= 4) {
				// Vertex position
				glm::vec3 pos = glm::vec3(stof(tokens[1],nullptr),stof(tokens[2], nullptr),stof(tokens[3], nullptr));
				geometry.positions.push_back(pos);
				
			} else if (tokens[0] == "vn" && tokens.size() >= 4){
				// Vertex normal
				glm::vec3 nor = glm::vec3(stof(tokens[1], nullptr),stof(tokens[2], nullptr),stof(tokens[3], nullptr));
				geometry.normals.push_back(nor);
				
			} else if (tokens[0] == "vt" && tokens.size() >= 3) {
				// Vertex UV
				glm::vec2 uv = glm::vec2(stof(tokens[1], nullptr),stof(tokens[2], nullptr));
				geometry.texcoords.push_back(uv);
				
			} else if (tokens[0] == "f" && tokens.size() >= 4) {
				// Face indices.
				geometry.faces.push_back(tokens[1]);
				geometry.faces.push_back(tokens[2]);
				geometry.faces.push_back(tokens[3]);
				faceNumber+=3;
				// If we have more vertices, insert additional faces.
				for(size_t i = 4; i < tokens.size(); ++i){
					geometry.faces.push_back(tokens[1]);
					geometry.faces.push_back(tokens[i-1]);
					geometry.faces.push_back(tokens[i]);
					faceNumber+=3;
				}
				
			} else if (tokens[0] == "g" || tokens[0] == "o"){
				// Handle groups and objects the same way.
				// Extract name if available.
				string objectName;
				if(tokens.size() < 2){
					objectName = "ObjectAt" + to_string(faceNumber);
				} else {
					TextUtilities::replace(tokens[1], "\\", "-");
					TextUtilities::replace(tokens[1], "/", "-");
					TextUtilities::replace(tokens[1], ":", "-");
					objectName = tokens[1] + "_" + std::to_string(faceNumber);
				}
				// Check if the previous object received any kind of geometry, if not, stay with the same object.
				if(objectMatUses.empty()){
					objectMatUses.emplace_back(objectName, "default", faceNumber);
					
				} else if(objectMatUses.back().index < faceNumber){
					const std::string currentMat = objectMatUses.back().materialName;
					objectMatUses.emplace_back(objectName, currentMat, faceNumber);
					
				} else {
					// Use the name of the new object, probably clearer.
					objectMatUses.back().objectName = objectName;
				}
				
			} else if(tokens[0] == "mtllib" && tokens.size() >= 2){
				// Register material library if it wasn't encountered before.
				TextUtilities::replace(tokens[1], "\\", "/");
				if(std::find(materialsFiles.begin(), materialsFiles.end(), tokens[1]) == materialsFiles.end()){
					materialsFiles.push_back(tokens[1]);
				}
				
			} else if(tokens[0] == "usemtl" && tokens.size() >= 2){
				// Register material use.
				TextUtilities::replace(tokens[1], "\\", "-");
				TextUtilities::replace(tokens[1], "/", "-");
				TextUtilities::replace(tokens[1], ":", "-");
				const std::string materialName = tokens[1];
				// A material can be:
				if(!objectMatUses.empty() && (faceNumber == objectMatUses.back().index)){
					// - pushed just after an object
					// In that case, replace the material of the last object.
					objectMatUses.back().materialName = materialName;
				} else {
					// - pushed in the middle of an object, in which case a new object is spawned.
					const std::string objectName = materialName + "_" + std::to_string(faceNumber);
					objectMatUses.emplace_back(objectName, materialName, faceNumber);
				}
			}
		}
	}


	/** Build a mesh from a subset of the raw geometry data.
	 \param geom the raw geoemtry data
	 \param lowerBound the index of the object first face
	 \param upperBound the index after the object last face
	 \param mesh will be updated with geoemtry data
	 */
	void populateMesh(const RawGeometry & geom, size_t lowerBound, size_t upperBound, Mesh & mesh){
		
		const bool hasUV = !geom.texcoords.empty();
		const bool hasNormals = !geom.normals.empty();
		
		mesh.indices.clear();
		mesh.positions.clear();
		mesh.normals.clear();
		mesh.texcoords.clear();
		
		// Keep track of previously encountered (position,uv,normal).
		std::map<std::string, long> indices_used;
		
		//Positions
		long maxInd = 0;
		for(size_t i = lowerBound; i < upperBound; ++i){
			const std::string str = geom.faces[i];
			
			//Does the association of attributs already exists ?
			if(indices_used.count(str)>0){
				// Just store the index in the indices vector.
				mesh.indices.push_back(indices_used[str]);
				// Go to next face.
				continue;
			}
			
			// else, query the associated position/uv/normal, store it, update the indices vector and the list of used elements.
			const size_t foundF = str.find_first_of('/');
			const size_t foundL = str.find_last_of('/');
			
			//Positions (we are sure they exist)
			const long ind1 = stol(str.substr(0,foundF))-1;
			mesh.positions.push_back(geom.positions[ind1]);
			
			//UVs (second index)
			if(hasUV){
				const long ind2 = stol(str.substr(foundF+1,foundL))-1;
				mesh.texcoords.push_back(geom.texcoords[ind2]);
			}
			//Normals (third index, in all cases)
			if(hasNormals){
				const long ind3 = stol(str.substr(foundL+1))-1;
				mesh.normals.push_back(geom.normals[ind3]);
			}
			
			mesh.indices.push_back(maxInd);
			indices_used[str] = maxInd;
			maxInd++;
		}
		indices_used.clear();
	}


	/** Parse a MTL file to extract materials.
	 \param inMat the stream to parse
	 \param rootPath the root path for all texture paths
	 \param materials contains the materials that might be udpated by this file
	 */
	void parseMtlFile(std::istream & inMat, const std::string & rootPath, std::map<std::string, Material> & materials){
		using namespace std;
		
		string resMat;
		string currentMaterialName;
		
		while(!inMat.eof()){
			getline(inMat,resMat);
			// Reject comments and short lines.
			if(resMat.size() < 6 || resMat[0] == '#'){
				continue;
			}
			//We want to split the content of the line at spaces, use a stringstream directly
			stringstream ss(resMat);
			vector<string> tokens;
			string token;
			while(ss >> token){
				tokens.push_back(token);
			}
			if(tokens.size() < 2){
				continue;
			}
			
			// Create new named material.
			if(tokens[0] == "newmtl"){
				TextUtilities::replace(tokens[1], "\\", "-");
				TextUtilities::replace(tokens[1], "/", "-");
				TextUtilities::replace(tokens[1], ":", "-");
				currentMaterialName = tokens[1];
				materials[currentMaterialName] = Material();
				
			} else if(tokens[0] == "map_Ka" || tokens[0] == "map_Kd"){
				// Diffuse/ambient color.
				TextUtilities::replace(tokens[1], "\\", "/");
				materials[currentMaterialName].colorTexturePath = rootPath + tokens[1];
				
			} else if (tokens[0] == "bump" || tokens[0] == "norm" || tokens[0] == "map_Bump" || tokens[0] == "map_bump"){
				// Normal map/bump map.
				TextUtilities::replace(tokens[1], "\\", "/");
				materials[currentMaterialName].normalTexturePath = rootPath + tokens[1];
				
			} else if (tokens[0] == "map_d"){
				// Alpha map.
				TextUtilities::replace(tokens[1], "\\", "/");
				materials[currentMaterialName].alphaTexturePath = rootPath + tokens[1];
				
			} else if (tokens[0] == "map_Ks" || tokens[0] == "map_Ns"){
				// Effects (specular) map.
				TextUtilities::replace(tokens[1], "\\", "/");
				materials[currentMaterialName].specTexturePath = rootPath + tokens[1];
				
			}  else if (tokens[0] == "map_disp"){
				TextUtilities::replace(tokens[1], "\\", "/");
				materials[currentMaterialName].displacementTexturePath = rootPath + tokens[1];
				
			} else if (tokens[0] == "map_Pr"){
				TextUtilities::replace(tokens[1], "\\", "/");
				materials[currentMaterialName].roughTexturePath = rootPath + tokens[1];
				
			} else if (tokens[0] == "map_Pm"){
				TextUtilities::replace(tokens[1], "\\", "/");
				materials[currentMaterialName].metalTexturePath = rootPath + tokens[1];
				
			} else if (tokens[0] == "Kd" && tokens.size() >= 4){
				const float r = std::stof(tokens[1]);
				const float g = std::stof(tokens[2]);
				const float b = std::stof(tokens[3]);
				materials[currentMaterialName].color = glm::vec3(r,g,b);
				materials[currentMaterialName].hasColor = true;
				
			} else if (tokens[0] == "Ks" && tokens.size() >= 4){
				const float r = std::stof(tokens[1]);
				const float g = std::stof(tokens[2]);
				const float b = std::stof(tokens[3]);
				if(r+g+b != 0.0f){
					materials[currentMaterialName].spec = (r+g+b)/3.0f;
					materials[currentMaterialName].hasSpec = true;
				}
			} else if (tokens[0] == "Ns"){
				const float n = std::stof(tokens[1]);
				materials[currentMaterialName].spec = n/1000.0f;
				materials[currentMaterialName].hasSpec = true;
				
			} else if (tokens[0] == "Pm"){
				const float n = std::stof(tokens[1]);
				materials[currentMaterialName].metal = n;
				materials[currentMaterialName].hasMetal = true;
				
			} else if (tokens[0] == "Pr"){
				const float n = std::stof(tokens[1]);
				materials[currentMaterialName].rough = n;
				materials[currentMaterialName].hasRough = true;
			}
		}
	}
	
	
	int load(const std::string & filePath, std::vector<Object>& objects, std::map<std::string, Material>& materials){
		
		using namespace std;
		
		ifstream objFile(filePath);
		if(!objFile.is_open()){
			Log::Error() << "Unable to load file at path \"" << filePath << "\"." << std::endl;
			return 2;
		}
		
		Log::Info() << Log::Resources << "Loading composite OBJ..." << std::endl;
		
		RawGeometry rawGeom;
		std::vector<std::string> materialsFiles;
		std::vector<ObjectMaterialUse> objMatUses;
		
		parseMultiObj(objFile, rawGeom, materialsFiles, objMatUses);
		
		// Done with the obj file, close it.
		objFile.close();
		
		// If no vertices, end.
		if(rawGeom.positions.empty()){
			Log::Warning() << Log::Resources << "No vertices found." << std::endl;
			return 3;
		}
		
		// Create a default object if none was defined.
		if(objMatUses.empty()){
			objMatUses.emplace_back("object", "default", 0);
		}
		
		// Build the final meshes.
		for(size_t j = 0; j < objMatUses.size(); ++j){
			// Generate a mesh for each object, with adjusted indices.
			auto& object = objMatUses[j];
			objects.emplace_back(object.objectName);
			objects.back().material = object.materialName;
			const size_t upperBound = (j == objMatUses.size() - 1) ? rawGeom.faces.size() : objMatUses[j+1].index;
			populateMesh(rawGeom, object.index, upperBound, objects.back().mesh);
		}
		// We are done with the raw geometry.
		rawGeom.clear();
		
		// Load materials files.
		const std::string::size_type sep = filePath.find_last_of("\\/");
		const string rootPath = filePath.substr(0, sep) + "/";
		
		// Parse each material library file.
		for(auto& materialFile : materialsFiles){
			const string materialFilePath = rootPath + materialFile;
			// Open the file.
			ifstream inMat(materialFilePath);
			if(!inMat.is_open()) {
				Log::Error() << materialFilePath + " is not a valid file." << endl;
				continue;
			}
			// Pass the whole vector as reference, as a library can contain multiple materials.
			parseMtlFile(inMat, rootPath, materials);
			inMat.close();
		}
		
		// Recap:
		Log::Info() << Log::Resources << "Found material files: " << endl;
		for(const auto & file : materialsFiles){
			Log::Info() << file << endl;
		}
		Log::Info() << Log::Resources << "Found objects: " << endl;
		for(const auto & use : objMatUses){
			Log::Info() << "* " << use.objectName << " at index " << use.index << " using " << use.materialName << endl;
		}
		return 0;
	}
	
}
