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
		raycaster.addMesh(obj.mesh()->geometry, obj.model());
	}
	raycaster.updateHierarchy();
	
	
	// Result image.
	Image render(1024, 1024, 3);
	// Setup camera.
	Camera camera;
	camera.pose(glm::vec3(0.0f, 1.0f, 2.5f), glm::vec3(0.0f, 1.0f, 2.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	camera.projection(1.0f, 1.3f, 0.01f, 100.0f);
	// Compute incremental pixel shifts.
	glm::vec3 corner, dx, dy;
	camera.pixelShifts(corner, dx, dy);
	
	// Start chrono.
	auto start = std::chrono::steady_clock::now();
	
	// Get the maximum number of threads.
	System::forParallel(0, render.height, [&](size_t y){
		for(unsigned int x = 0; x < render.width; ++x){
			
			// Derive a position on the image plane from the pixel.
			const glm::vec2 ndcPos = (glm::vec2(x,y) + 0.5f) / glm::vec2(render.width, render.height);
			// Place the point on the near plane in clip space.
			const glm::vec3 worldPos = corner + ndcPos.x * dx + ndcPos.y * dy;
			const glm::vec3 worldDir = glm::normalize(worldPos - camera.position());
			// Query closest intersection.
			const Raycaster::RayHit hit = raycaster.intersects(camera.position(), worldDir);
			// If no hit, background.
			if(!hit.hit){
				render.rgb(x,y) = glm::vec3(0.0f, 0.0f, 0.0f);
				continue;
			}
			
			// Fetch geometry infos...
			const Mesh & mesh = scene.objects[hit.meshId].mesh()->geometry;
			const glm::vec3 p = camera.position() + hit.dist * worldDir;
			const glm::vec3 n = Raycaster::interpolateNormal(hit, mesh);
			const glm::vec2 uv = Raycaster::interpolateUV(hit, mesh);
			
			// Compute lighting.
			// Check light visibility.
			glm::vec3 illumination(0.1f);
			for(const auto light : scene.lights){
				glm::vec3 direction;
				float attenuation;
				if(light->visible(p, raycaster, direction, attenuation)){
					const float diffuse = glm::max(glm::dot(n, direction), 0.0f);
					illumination += attenuation * diffuse * light->intensity();
				}
			}
			
			// Fetch base color from texture.
			const Image & image = scene.objects[hit.meshId].textures()[0]->images[0];
			const int px = int(std::floor(uv.x * float(image.width))) % image.width;
			const int py = int(std::floor(uv.y * float(image.height))) % image.height;
			const glm::vec3 & baseColor = image.rgbc(px, py);
			// Modulate and store, applying gamma correction.
			render.rgb(x,y) = glm::pow(illumination * baseColor, glm::vec3(1.0f/2.2f));
		}
	});
	
	// Display duration.
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
	Log::Info() << "Generation took " << duration.count() << " ms at " << render.width << "x" << render.height << "." << std::endl;
	
	// Save image.
	ImageUtilities::saveLDRImage("./test.png", render, false);
	return 0;
}


