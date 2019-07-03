#include "Common.hpp"
#include "raycaster/Raycaster.hpp"
#include "helpers/Random.hpp"
#include "input/Input.hpp"
#include "input/InputCallbacks.hpp"
#include "input/ControllableCamera.hpp"
#include "helpers/Interface.hpp"
#include "resources/ResourcesManager.hpp"
#include "graphics/ScreenQuad.hpp"
#include "Config.hpp"
#include <thread>

/**
 \defgroup RaytracerDemo Raytracer demo
 \brief A basic ray tracing demo.
 \ingroup Applications
 */

/**
 The main function of the demo.
 \param argc the number of input arguments.
 \param argv a pointer to the raw input arguments.
 \return a general error code.
 \ingroup RaytracerDemo
 */
int main(int argc, char** argv) {
	
	// Initialize random generator;
	Random::seed();
	Resources::manager().addResources("../../../resources/pbrdemo");
	
	// Load geometry and create raycaster.
	const MeshInfos * mesh = Resources::manager().getMesh("dragon", Storage::CPU);
	Log::Info() << mesh->geometry.positions.size() << " vertices, " << mesh->geometry.indices.size()/3 << " triangles." << std::endl;
	Raycaster raycaster;
	raycaster.addMesh(mesh->geometry);
	raycaster.updateHierarchy();
	
	// Load model texture.
	TextureInfos *texture = Resources::manager().getTexture("dragon_texture_color", {}, Storage::CPU);
	Image & image = texture->images[0];
	// Light direction.
	const glm::vec3 l = glm::normalize(glm::vec3(1.0));
	
	// Result image.
	Image render(512, 512, 3);
	// Setup camera.
	Camera camera;
	camera.pose(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	camera.projection(1.0, 2.5f, 0.01f, 100.0f);
	// We only care about the X,Y transformation of the projection matrix.
	const glm::mat3 invP = glm::mat3(glm::inverse(glm::mat2(camera.projection())));
	const glm::mat3 invVP = glm::inverse(glm::mat3(camera.view())) * invP;
	
	// Start chrono.
	auto start = std::chrono::steady_clock::now();
	
	// Get the maximum number of threads.
	const size_t threadsCount = std::max(std::thread::hardware_concurrency(), unsigned(1));
	std::vector<std::thread> threads(threadsCount);
	for(size_t tid = 0; tid < threadsCount; ++tid){
		// For each thread, create the same lambda, with different loop bounds values passed as arguments.
		const unsigned int loopLow = tid * render.height / threadsCount;
		const unsigned int loopHigh = (tid == threadsCount - 1) ? render.height : (tid + 1) * render.height/threadsCount;
		threads[tid] = std::thread(std::bind( [&](const unsigned int lo, const unsigned int hi) {
	
			// This is the loop we want to parallelize.
			// Loop over all the pixels.
			for(unsigned int y = lo; y < hi; ++y){
				for(unsigned int x = 0; x < render.width; ++x){
					// Derive a position on the image plane from the pixel.
					glm::vec2 ndcSpace = 2.0f*(glm::vec2(x,y) + 0.5f) / glm::vec2(render.width, render.height) - 1.0f;
					ndcSpace.y *= -1.0f;
					// Place the point on the near plane in clip space.
					const glm::vec3 worldPos = invVP * glm::vec3(ndcSpace, -1.0f);
					const glm::vec3 worldDir = worldPos - camera.position();
					// Query closest intersection.
					const Raycaster::RayHit hit = raycaster.intersects(camera.position(), worldDir);
					// If no hit, background.
					if(!hit.hit){
						render.rgb(x,y) = glm::vec3(0.0f, 0.0f, 0.0f);
						continue;
					}
					
					// Else, compute third barycentric coordinates...
					const float w = 1.0f - hit.u - hit.v;
					// Fetch geometry infos...
					const unsigned long triangleId = hit.localId;
					const unsigned long i0 = mesh->geometry.indices[triangleId];
					const unsigned long i1 = mesh->geometry.indices[triangleId+1];
					const unsigned long i2 = mesh->geometry.indices[triangleId+2];
					// And interpolate the UVs and normal.
					const glm::vec2 uv  = w * mesh->geometry.texcoords[i0]
										+ hit.u * mesh->geometry.texcoords[i1]
										+ hit.v * mesh->geometry.texcoords[i2];
					const glm::vec3 n = glm::normalize(w * mesh->geometry.normals[i0] + hit.u * mesh->geometry.normals[i1] + hit.v * mesh->geometry.normals[i2]);
					// Compute lighting.
					const float diffuse = std::max(0.0f, glm::dot(n, l));
					// Fetch base color from texture.
					const glm::vec3 baseColor = image.rgb(std::floor(uv.x * image.width), std::floor(uv.y * image.height));
					// Done.
					render.rgb(x,y) = diffuse * baseColor;
				}
			}
		}, loopLow, loopHigh));
	}
	// Wait for all threads to finish.
	std::for_each(threads.begin(),threads.end(),[](std::thread& x){x.join();});
	// Display duration.
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
	Log::Info() << "Generation took " << duration.count() << " ms at " << render.width << "x" << render.height << "." << std::endl;
	
	// Save image.
	ImageUtilities::saveLDRImage("./test.png", render, false);
	return 0;
}


