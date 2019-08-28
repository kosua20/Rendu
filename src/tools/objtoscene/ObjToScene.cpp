
#include "system/System.hpp"
#include "system/TextUtilities.hpp"
#include "CompositeObj.hpp"
#include "SceneExport.hpp"
#include "Common.hpp"

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
	explicit ObjToSceneConfig(const std::vector<std::string> & argv) : Config(argv) {
		for (const auto & arg : arguments()) {
			const std::string key = arg.key;
			const std::vector<std::string> & values = arg.values;

			if (key == "mesh" && !values.empty()) {
				inputMeshPath = values[0];
			}
			else if (key == "output" && !values.empty()) {
				outputPath = values[0];
			}
			else if (key == "name" && !values.empty()) {
				outputName = values[0];
			}
			else if (key == "generate" && values.size() >= 3) {
				generateMap = true;
				const float r = std::stof(values[0]);
				const float m = std::stof(values[1]);
				const float ao = std::stof(values[2]);
				valuesMap = glm::vec3(r, m, ao);
			}
		}


		infos().emplace_back("", "", "Converter");
		infos().emplace_back("mesh", "", "Path to the OBJ file", "path/to/mesh.obj");
		infos().emplace_back("output", "", "Output path", "path");
		infos().emplace_back("name", "", "The name of the scene", "name");
		infos().emplace_back("generate", "", "Generate an image containing given color", "R G B");
	}
	
public:
	
	std::string inputMeshPath; ///< Input OBJ path. Textures paths should be relative to it.
	std::string outputPath = "./"; ///< Output directory path. Should already exists.
	std::string outputName = "scene"; ///< Scene name, will be used as a prefix for all files.
	
	bool generateMap = false; ///< Generate a RGB color 8x8 image.
	glm::vec3 valuesMap = glm::vec3(0.5f,0.0f,1.0f); ///< Values for the generated image.
	
};


/**
 Load a complex multi-objects multi-materials OBJ file and generate a Rendu scene from it,
 outputing meshes, material textures and the scene description file.
 \param argc the number of input arguments.
 \param argv a pointer to the raw input arguments.
 \return a general error code.
 \ingroup ObjToScene
 */
int main(int argc, char** argv) {
	// First, init/parse/load configuration.
	ObjToSceneConfig config(std::vector<std::string>(argv, argv+argc));
	if(config.showHelp()){
		return 0;
	}
	
	// Export basic color map.
	if(config.generateMap && !config.outputPath.empty()){
		SceneExport::saveColor(config.outputPath, config.valuesMap);
		return 0;
	}
	
	if(config.inputMeshPath.empty() || config.outputPath.empty()){
		Log::Error() << "No file passed as input/output." << std::endl;
		return 1;
	}
	
	System::createDirectory(config.outputPath);
	
	// Load the meshes and materials.
	std::vector<CompositeObj::Object> objects;
	std::map<std::string, CompositeObj::Material> materials;
	const int ret = CompositeObj::load(config.inputMeshPath, objects, materials);
	if(ret != 0){
		return ret;
	}
	
	Log::Info() << Log::Resources << "Loaded " << objects.size() << " meshes, " << materials.size() << " materials." << std::endl;
	
	// Save each mesh, computing normals if needed.
	for(auto & object : objects){
		if(object.mesh.normals.empty()){
			Mesh::computeNormals(object.mesh);
		}
		// Export the mesh.
		object.name = config.outputName + "_" + object.name;
		const std::string filePath = config.outputPath + "/" + object.name + ".obj";
		Mesh::saveObj(filePath, object.mesh, true);
	}
	
	// Save each material, creating textures if needed.
	std::map<std::string, SceneExport::Material> finalMaterials;
	for(auto & materialKey : materials){
		const std::string baseName = config.outputName + "_" + materialKey.first;
		finalMaterials[materialKey.first] = SceneExport::saveMaterial(baseName, materialKey.second, config.outputPath);
	}
	
	// Save the scene file.
	const int ret1 = SceneExport::saveDescription(objects, finalMaterials, config.outputPath + config.outputName + ".scene");
	return ret1;
}
