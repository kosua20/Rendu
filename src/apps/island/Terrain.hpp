
#include "resources/Image.hpp"
#include "resources/Texture.hpp"
#include "resources/Mesh.hpp"
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
		float scale = 0.02f;
		float maxHeight = 2.5f;
		float falloff = 4.0f;
		float rescale = 1.5f;
		int octaves = 8;
	};

	struct MeshSettings {
		int size = 112;
		int levels = 4;
	};
	/** Constructor
	 \param config rendering config
	 */
	Terrain(uint resolution, uint seed);

	void generateMesh();

	void generateMap();

	void interface();

	void clean();

	float texelSize() const {
		return _texelSize;
	}

	int gridSize() const {
		return _mshOpts.size;
	}

	const Texture & map() const {
		return _map;
	}

	const Mesh & mesh() const {
		return _mesh;
	}

private:
	PerlinNoise _perlin;
	Mesh _mesh;
	Texture _map;

	GenerationSettings _genOpts;
	MeshSettings _mshOpts;
	int _resolution;
	uint _seed;
	float _texelSize = 0.05f;

};
