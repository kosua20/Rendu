#pragma once

#include "system/Config.hpp"
#include "Common.hpp"

#include <thread>

/**
 \brief Performs system basic operations such as directory creation, timing, threading, file picking.
 \ingroup System
 */
class System {
public:
	/** The file picker mode. */
	enum class Picker {
		Load,	  ///< Load an existing file.
		Directory, ///< open or create a directory.
		Save	   ///< Save to a new or existing file.
	};

	/** Present a filesystem document picker to the user, using native controls.
		 \param mode the type of item to ask to the user (load, save, directory)
		 \param startDir the initial directory when the picker is opened
		 \param outPath the path to the item selected by the user
		 \param extensions (optional) the extensions allowed, separated by "," or ";"
		 \return true if the user picked an item, false if cancelled.
		 */
	static bool showPicker(Picker mode, const std::string & startDir, std::string & outPath, const std::string & extensions = "");

	/** Create a directory.
		 \param directory the path to the directory to create
		 \return true if the creation is successful.
		 \note If the directory already exists, it will fail.
		 \warning This function will not create intermediate directories.
		 */
	static bool createDirectory(const std::string & directory);

	/** Notify the user by sending a 'Bell' signal. */
	static void ping();
	
	/** Return the current value of a time counter.
	 \return the current counter value, in seconds.
	 */
	static double time();

	/** Obtain a YYYY_MM_DD_HH_MM_SS timestamp of the current time.
	 \return the string representation
	 */
	static std::string timestamp();

	/** Multi-threaded for-loop.
		 \param low lower (included) bound
		 \param high higher (excluded) bound
		 \param func the function to execute at each iteration, will receive the index of the
		 element as a unique argument. Signature: void func(size_t i)
		 \note For now only an increment by one is supported.
		 */
	template<typename ThreadFunc>
	static void forParallel(size_t low, size_t high, ThreadFunc func) {
		// Make sure the loop is increasing.
		if(high < low) {
			const size_t temp = low;
			low				  = high;
			high			  = temp;
		}
		// Prepare the threads pool.
		// Always leave one thread free.
		const size_t availableThreadsCount = std::max(int(std::thread::hardware_concurrency())-1, 1);
		const size_t count = std::min(size_t(availableThreadsCount), high - low);
		std::vector<std::thread> threads;
		threads.reserve(count);

		// Compute the span of each thread.
		const size_t span = std::max(size_t(1), (high - low) / count);
		// Helper to execute the function passed on a subset of the total interval.
		auto launchThread = [&func](size_t a, size_t b) {
			for(size_t i = a; i < b; ++i) {
				func(i);
			}
		};

		for(size_t tid = 0; tid < count; ++tid) {
			// For each thread, call the same lambda with different bounds as arguments.
			const size_t threadLow  = tid * span;
			const size_t threadHigh = tid == (count-1) ? high : ((tid + 1) * span);
			threads.emplace_back(launchThread, threadLow, threadHigh);
		}
		// Wait for all threads to finish.
		std::for_each(threads.begin(), threads.end(), [](std::thread & x) { x.join(); });
	}

	#ifdef _WIN32

	/** Convert a string to the system representation.
	 \param str a UTF8 standard string
	 \return the corresponding system string
	 */
	static wchar_t * widen(const std::string & str);
	
	/** Convert a string from the system representation.
	 \param str a system string
	 \return the corresponding UTF8 standard string
	 */
	static std::string narrow(wchar_t * str);

	#else

	/** Convert a string to the system representation.
	 \param str a UTF8 standard string
	 \return the corresponding system string
	 */
	static const char * widen(const std::string & str);
	
	/** Convert a string from the system representation.
	 \param str a system string
	 \return the corresponding UTF8 standard string
	 */
	static std::string narrow(char * str);

	#endif

};
