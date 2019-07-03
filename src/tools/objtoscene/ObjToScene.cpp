#include "Common.hpp"
#include "helpers/Interface.hpp"
#include "helpers/TextUtilities.hpp"
#include "MultiObjLoader.hpp"
#include "SceneWriter.hpp"

/**
 \defgroup ObjToScene OBJ to scene converter
 \brief Process a multi-objects/groups/materials OBJ file to generate a Rendu scene on disk.
 \ingroup Tools
 */


/**
 \brief Configuration for the OBJ to scene converter.
 \ingroup ObjToScene
 */
class ObjToSceneConfig : public Config {
public:
	
	/** Initialize a new config object, parsing the input arguments and filling the attributes with their values.
	 \param argv the raw input arguments
	 */
	ObjToSceneConfig(const std::vector<std::string> & argv) : Config(argv) {
		processArguments();
	}
	
	/**
	 Read the internal (key, [values]) populated dictionary, and transfer their values to the configuration attributes.
	 */
	void processArguments(){
		for(const auto & arg : _rawArguments){
			const std::string key = arg.key;
			const std::vector<std::string> & values = arg.values;
			
			if(key == "mesh" && !values.empty()){
				inputMeshPath = values[0];
			} else if(key == "output" && !values.empty()){
				outputDirPath = values[0] + "/";
			} else if(key == "name" && !values.empty()){
				outputName = values[0];
			} else if(key == "generate-rmo"){
				generateRMO = true;
			} else if(key == "rmo" && values.size() >= 3){
				const float r = std::stof(values[0]);
				const float m = std::stof(values[1]);
				const float ao = std::stof(values[2]);
				valuesRMO = glm::vec3(r,m,ao);
			}
		}
	}
	
public:
	
	std::string inputMeshPath; ///< Input OBJ path. Textures paths should be relative to it.
	std::string outputDirPath = "./"; ///< Output directory path. Should already exists.
	std::string outputName = "scene"; ///< Scene name, will be used as a prefix for all files.
	
	bool generateRMO = false; ///< Generate a roughness-metalness-ambient occlusion 8x8 image.
	glm::vec3 valuesRMO = glm::vec3(0.5f,0.0f,1.0f); ///< Values for the generated image.
	
};


/**

 \param argc the number of input arguments.
 \param argv a pointer to the raw input arguments.
 \return a general error code.
 \ingroup SceneCreator
 */
int main(int argc, char** argv) {
	// First, init/parse/load configuration.
	ObjToSceneConfig config(std::vector<std::string>(argv, argv+argc));
	
	if(config.inputMeshPath.empty() || config.outputDirPath.empty()){
		Log::Error() << "No file passed as input/output." << std::endl;
		return 1;
	}
	
	// Load the meshes and materials.
	std::vector<ObjMaterialMesh> objects;
	std::map<std::string, ObjMaterial> materials;
	const int ret = loadCompositeObj(config.inputMeshPath, objects, materials);
	if(ret != 0){
		return ret;
	}
	
	Log::Info() << Log::Resources << "Loaded " << objects.size() << " meshes, " << materials.size() << " materials." << std::endl;
	
	// Save each mesh, computing normals if needed.
	for(auto & object : objects){
		const std::string filePath = config.outputDirPath + object.name + ".obj";
		saveMesh(object.mesh, filePath);
		object.mesh.clear();
	}
	
	// Save each material, creating textures if needed.
	std::map<std::string, FinalMaterialInfos> finalMaterials;
	for(auto & materialKey : materials){
		const std::string prefix = config.outputName + "_" + materialKey.first;
		finalMaterials[materialKey.first] = saveMaterial(prefix, materialKey.second, config.outputDirPath);
	}
	
	// Save the scene file.
	const int ret1 = saveSceneFile(objects, finalMaterials, config.outputDirPath + config.outputName + ".scene");
	return ret1;
}
