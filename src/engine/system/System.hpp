#pragma once

#include "system/Config.hpp"
#include "Common.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <thread>

struct GLFWwindow;

/**
 \brief Performs system basic operations, file picking, interface setup and loop.
 \ingroup Helpers
 */
namespace System {
	
	/**
	 \brief GUI helpers, internally backed by ImGUI.
	 \ingroup Helpers
	 */
	namespace GUI {
	
		/** Start registering GUI items. */
		void beginFrame();
	
		/** Finish registering GUI items and render them. */
		void endFrame();
		
		/** Clean internal resources. */
		void clean();
	
	}
	
	/** Create a new window backed by an OpenGL context.
	 \param name the name of the window
	 \param config the configuration to use (additional info will be added to it)
	 \return a pointer to the OS window
	 */
	GLFWwindow* initWindow(const std::string & name, RenderingConfig & config);
	
	/** System actions that can be executed by the window. */
	enum class Action {
		None, ///< Do nothing.
		Quit, ///< Quit the application.
		Fullscreen, ///< Switch the window from/to fullscreen mode.
		Vsync ///< Switch the v-sync on/off.
	};
	
	/** Execute an action related to the windowing system.
	 \param window the window
	 \param config the current window configuration
	 \param action the system action to perform
	 */
	void performWindowAction(GLFWwindow * window, RenderingConfig & config, const Action action);
	
	
	/** The file picker mode. */
	enum class Picker {
		Load, ///< Load an existing file.
		Directory, ///< open or create a directory.
		Save ///< Save to a new or existing file.
	};
	
	/** Present a filesystem document picker to the user, using native controls.
	 \param mode the type of item to ask to the user (load, save, directory)
	 \param startDir the initial directory when the picker is opened
	 \param outPath the path to the item selected by the user
	 \param extensions (optional) the extensions allowed, separated by "," or ";"
	 \return true if the user picked an item, false if cancelled.
	 */
	bool showPicker(const Picker mode, const std::string & startDir, std::string & outPath, const std::string & extensions = "");

	/** Create a directory.
	 \param directory the path to the directory to create
	 \return true if the creation is successful.
	 \note If the directory already exists, it will fail.
	 \warning This function will not create intermediate directories.
	 */
	bool createDirectory(const std::string & directory);
	
	/** Notify the user by sending a 'Bell' signal. */
	void ping();
	
	/** Multi-threaded for-loop.
	 \param low lower (included) bound
	 \param high higher (excluded) bound
	 \param func the function to execute at each iteration, will receive the index of the
	 element as a unique argument. Signature: void func(size_t i)
	 \note For now only an increment by one is supported.
	 */
	template<typename ThreadFunc>
	void forParallel(size_t low, size_t high, ThreadFunc func){
		// Make sure the loop is increasing.
		if(high < low){
			const size_t temp = low;
			low = high;
			high = temp;
		}
		// Prepare the threads pool.
		const size_t count = size_t(std::max(std::thread::hardware_concurrency(), unsigned(1)));
		std::vector<std::thread> threads;
		threads.reserve(count);
		
		// Compute the span of each thread.
		size_t span = size_t(std::round((float(high) - float(low))/float(count)));
		span = std::max(size_t(1), span);
		
		// Helper to execute the function passed on a subset of the total interval.
		auto launchThread = [&func](size_t a, size_t b) {
			for(size_t i = a; i < b; ++i) {
				func(i);
			}
		};
		
		for(size_t tid = 0; tid < count; ++tid){
			// For each thread, call the same lambda with different bounds as arguments.
			const size_t threadLow = tid * span;
			size_t threadHigh = (tid == count - 1) ? high : ((tid+1) * span);
			threads.emplace_back(launchThread, threadLow, threadHigh);
		}
		// Wait for all threads to finish.
		std::for_each(threads.begin(),threads.end(),[](std::thread& x){x.join();});
	}


#ifdef _WIN32

	WCHAR * widen(const std::string & str);

	std::string narrow(WCHAR * str);

#else

	const char * widen(const std::string & str);

	std::string narrow(char * str);

#endif
	
}

