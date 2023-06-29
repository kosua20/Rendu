
#include "resources/Image.hpp"
#include "resources/Texture.hpp"
#include "resources/Mesh.hpp"
#include "processing/GaussianBlur.hpp"
#include "generation/PerlinNoise.hpp"
#include "generation/Random.hpp"
#include "Common.hpp"

/** \brief Generate a terrain with Perlin noise and erosion.
 Represent the terrain, regrouping elevation and shadow data and the underlying GPU representation to render it.
 \ingroup Island
 */
class Terrain {
public:

	/** Terrain mesh cell. */
	struct Cell {
		Mesh mesh; ///< The geometry covering this cell.
		uint level; ///< The density level used to generate the mesh of this cell.

		/** Constructor.
		 \param l the level of detail
		 \param x cell x coordinate
		 \param z cell z coordinate
		 */
		Cell(uint l, uint x, uint z);
	};

	/** Constructor
	 \param resolution the terrain map resolution
	 \param seed the random seed for terrain generation
	 */
	Terrain(uint resolution, uint seed);

	/** Generate the grid mesh.*/
	void generateMesh();

	/** Generate the terrain map for the current seed. */
	void generateMap();

	/** Generate the shadow map for the current terrain and a sun direction.
	 \param lightDir the sun direction
	 */
	void generateShadowMap(const glm::vec3 & lightDir);

	/** Display terrain options in GUI, in a currently opened window.
	 \return true if any option was modified
	 */
	bool interface();

	/** Destructor. */
	~Terrain();

	/** \return the size of a terrain map pixel in world space. */
	float texelSize() const {
		return _texelSize;
	}

	/** \return the size of the grid. */
	int gridSize() const {
		return _mshOpts.size;
	}

	/** \return the size of the grid in world space. */
	float meshSize() const {
		return _meshSize * _texelSize;
	}

	/** \return the terrain height and normal map. */
	const Texture & map() const {
		return _map;
	}

	/** \return the terrain shadow map, with self shadowing in the R channel and ocean plane shadowing in the G channel. */
	const Texture & shadowMap() const {
		return _shadowMap;
	}

	/** \return the grid mesh cells. */
	const std::vector<Cell> & cells() const {
		return _cells;
	}

private:

	/** Apply erosion on a height map.
	 \param img the map to erode, in place
	 */
	void erode(Image & img);

	/** Compute terrain normals from height and uplaod the result to the GPU, with custom mip-map and low-res version.
	 \param heightMap the map to update and upload
	 */
	void transferAndUpdateMap(Image & heightMap);

	/** Noise map generation options. */
	struct GenerationSettings {
		float lacunarity = 2.0f; ///< The frequency ratio between successive noise layers.
		float gain = 0.5f; ///< The amplitude ratio between successive noise layers.
		float scale = 0.01f; ///< The base frequency, in pixels.
		float maxHeight = 2.0f; ///< Maximum height.
		float falloff = 4.0f; ///< Strength of the circular falloff.
		float rescale = 1.25f; ///< General height rescaling.
		int octaves = 8; ///< Number of noise layers.
	};

	/** Grid mesh options. */
	struct MeshSettings {
		int size = 96; ///< Grid dimensions.
		int levels = 4; ///< Number of levels of detail.
	};

	/** Map erosion options. */
	struct ErosionSettings {
		float inertia = 0.6f; ///< Inertia of water droplet.
		float gravity = 10.0f; ///< Gravity.
		float minSlope = 0.01f; ///< Minimal slope, to avoid strong erosion spots.
		float capacityBase = 12.0f; ///< Base water capacity.
		float erosion = 0.75f; ///< Erosion strenght.
		float evaporation = 0.02f; ///< Evaporation speed.
		float deposition = 0.2f; ///< Deposition speed.
		int gatherRadius = 3; ///< Gathering radius for contributions.
		int dropsCount = 50000; ///< Number of droplets to sequentially simulate.
		int stepsMax = 256; ///< Number of steps for each droplet simulation.
		bool apply = true; ///< Should erosion be applied.
	};

	PerlinNoise _perlin; ///< Perlin noise generator.
	std::vector<Cell> _cells; ///< Grid mesh cells.
	Texture _map = Texture("Terrain"); ///< Terrain map, height in R channel, normals in GBA channels.
	Texture _mapLowRes = Texture("Terrain low-res");///< Low resolution min-height terrain map.
	Texture _shadowMap; ///< Shadow map generation texture.
	GaussianBlur _gaussBlur; ///< Gaussian blur.

	GenerationSettings _genOpts; ///< Terrain generations settings.
	MeshSettings _mshOpts; ///< Grid mesh options.
	ErosionSettings _erOpts; ///< Erosion options.

	int _resolution; ///< Height map resolution.
	uint _seed; ///< Current generation seed.
	float _texelSize = 0.05f; ///< Size of a map texel in world space.
	float _meshSize = 0.0f; ///< Size of the mesh.

};
