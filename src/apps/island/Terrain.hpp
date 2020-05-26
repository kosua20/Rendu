
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


	struct MeshSettings {
		int size = 112;
		int levels = 4;
	};
	/** Constructor
	 \param config rendering config
	 */
	Terrain(uint resolution, uint seed);

	void generateMesh();


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
	Mesh _mesh;
	Texture _map;

	MeshSettings _mshOpts;
	int _resolution;
	uint _seed;
	float _texelSize = 0.05f;

};
