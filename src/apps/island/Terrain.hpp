
#include "resources/Image.hpp"
#include "resources/Texture.hpp"
#include "resources/Mesh.hpp"
#include "processing/GaussianBlur.hpp"
#include "graphics/Framebuffer.hpp"
#include "generation/PerlinNoise.hpp"
#include "generation/Random.hpp"
#include "Common.hpp"

/** \brief
 \ingroup Island
 */
class Terrain {
public:

	// Terrain options.
	struct GenerationSettings {
		float lacunarity = 2.0f;
		float gain = 0.5f;
		float scale = 0.01f;
		float maxHeight = 2.0f;
		float falloff = 4.0f;
		float rescale = 1.25f;
		int octaves = 8;
		bool flatten = false;
	};

	struct MeshSettings {
		int size = 96;
		int levels = 4;
	};

	struct ErosionSettings {
		float inertia = 0.6f;
		float gravity = 10.0f;
		float minSlope = 0.01f;
		float capacityBase = 12.0f;
		float erosion = 0.75f;
		float evaporation = 0.02f;
		float deposition = 0.2f;
		int gatherRadius = 3;
		int dropsCount = 50000;
		int stepsMax = 256;
		bool apply = true;
	};

	struct Cell {
		Mesh mesh;
		uint level;
	};

	/** Constructor
	 \param config rendering config
	 */
	Terrain(uint resolution, uint seed);

	void generateMesh();

	void generateMap();

	void generateShadowMap(const glm::vec3 & lightDir);

	bool interface();

	~Terrain();

	float texelSize() const {
		return _texelSize;
	}

	int gridSize() const {
		return _mshOpts.size;
	}

	float meshSize() const {
		return _meshSize * _texelSize;
	}

	const Texture & map() const {
		return _map;
	}

	const Texture * shadowMap() const {
		return _shadowBuffer->texture(0);
	}

	const std::vector<Cell> & cells() const {
		return _cells;
	}

private:

	void erode(Image & img);

	void transferAndUpdateMap(Image & heightMap);

	PerlinNoise _perlin;
	std::vector<Cell> _cells;
	Texture _map = Texture("Terrain");
	Texture _mapLowRes = Texture("Terrain low-res");
	std::unique_ptr<Framebuffer> _shadowBuffer;
	GaussianBlur _gaussBlur;

	GenerationSettings _genOpts;
	MeshSettings _mshOpts;
	ErosionSettings _erOpts;

	int _resolution;
	uint _seed;
	float _texelSize = 0.05f;
	float _meshSize = 0.0f;

};
