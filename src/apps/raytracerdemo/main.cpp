#include "Common.hpp"
#include "Config.hpp"

#include "raycaster/Raycaster.hpp"

#include "input/Input.hpp"
#include "input/InputCallbacks.hpp"
#include "input/ControllableCamera.hpp"

#include "scene/Scene.hpp"
#include "resources/ResourcesManager.hpp"
#include "graphics/ScreenQuad.hpp"

#include "helpers/Random.hpp"
#include "helpers/System.hpp"

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
	Resources::manager().addResources("../../../resources/additional");
	
	// Load geometry and create raycaster.
	Scene scene("cornellbox");
	scene.init(Storage::CPU);
	
	Raycaster raycaster;
	for(const auto & obj : scene.objects){
		raycaster.addMesh(obj.mesh()->geometry);
	}
	raycaster.updateHierarchy();
	
	
	// Light direction.
	const glm::vec3 l = glm::normalize(glm::vec3(1.0));
	
	// Result image.
	Image render(512, 512, 3);
	// Setup camera.
	Camera camera;
	camera.pose(glm::vec3(0.0f, 1.0f, 2.0f), glm::vec3(0.0f, 1.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	camera.projection(1.0, 2.5f, 0.01f, 100.0f);
	// We only care about the X,Y transformation of the projection matrix.
	const glm::mat3 invP = glm::mat3(glm::inverse(glm::mat2(camera.projection())));
	const glm::mat3 invVP = glm::inverse(glm::mat3(camera.view())) * invP;
	
	// Start chrono.
	auto start = std::chrono::steady_clock::now();
	
	// Get the maximum number of threads.
	System::forParallel(0, render.height, [&](size_t y){
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
			
			// Fetch geometry infos...
			const unsigned long meshId = hit.meshId;
			const auto * mesh = scene.objects[meshId].mesh();
			const glm::vec3 n = Raycaster::interpolateNormal(hit, mesh->geometry);
			const glm::vec2 uv = Raycaster::interpolateUV(hit, mesh->geometry);
			
			// Compute lighting.
			const float diffuse = std::max(0.0f, glm::dot(n, l)) + 0.1;
			// Fetch base color from texture.
			const Image & image = scene.objects[meshId].textures()[0]->images[0];
			const int px = int(std::floor(uv.x * float(image.width)));
			const int py = int(std::floor(uv.y * float(image.height)));
			const glm::vec3 baseColor = image.rgbc(px, py);
			// Modulate and store.
			render.rgb(x,y) = diffuse * baseColor;
		}
	});
	
	// Display duration.
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
	Log::Info() << "Generation took " << duration.count() << " ms at " << render.width << "x" << render.height << "." << std::endl;
	
	// Save image.
	ImageUtilities::saveLDRImage("./test.png", render, false);
	return 0;
}


