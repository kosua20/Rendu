#include "system/System.hpp"

#include <nfd/nfd.h>

#include <GLFW/glfw3.h>

#ifndef _WIN32
#	include <sys/stat.h>
#else
# 	undef APIENTRY
#	include <Windows.h>
#endif

#define XXH_INLINE_ALL
#include <xxhash/xxhash.h>

// On Windows, we can notify both AMD and Nvidia drivers that we prefer discrete GPUs.
#ifdef _WIN32
extern "C" {
	// See https://gpuopen.com/learn/amdpowerxpressrequesthighperformance/
	_declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001;
	// See https://docs.nvidia.com/gameworks/content/technologies/desktop/optimus.htm
	_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}
#endif

bool System::showPicker(Picker mode, const std::string & startDir, std::string & outPath, const std::string & extensions) {
	nfdchar_t * outPathRaw = nullptr;
	nfdresult_t result	 = NFD_CANCEL;
	outPath				   = "";

#ifdef _WIN32
	(void)startDir;
	const std::string internalStartPath;
#else
	const std::string internalStartPath = startDir;
#endif
	if(mode == Picker::Load) {
		result = NFD_OpenDialog(extensions.empty() ? nullptr : extensions.c_str(), internalStartPath.c_str(), &outPathRaw);
	} else if(mode == Picker::Save) {
		result = NFD_SaveDialog(extensions.empty() ? nullptr : extensions.c_str(), internalStartPath.c_str(), &outPathRaw);
	} else if(mode == Picker::Directory) {
		result = NFD_PickFolder(internalStartPath.c_str(), &outPathRaw);
	}

	if(result == NFD_OKAY) {
		outPath = std::string(outPathRaw);
		free(outPathRaw);
		return true;
	}
	if(result == NFD_CANCEL) {
		// Cancelled by user, nothing to do.
	} else {
		// Real error.
		Log::Error() << "Unable to present system picker (" << std::string(NFD_GetError()) << ")." << std::endl;
	}
	free(outPathRaw);
	return false;
}

#ifdef _WIN32

bool System::createDirectory(const std::string & directory) {
	return CreateDirectoryW(widen(directory), nullptr) != 0;
}

#else

bool System::createDirectory(const std::string & directory) {
	return mkdir(directory.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == 0;
}

#endif

void System::ping() {
	Log::Info() << '\a' << std::endl;
}

double System::time(){
	return glfwGetTime();
}

std::string System::timestamp(){
	const auto time = std::time(nullptr);
#ifdef _WIN32
	tm ltime = { 0,0,0,0,0,0,0,0,0 };
	localtime_s(&ltime, &time);
#else
	const tm ltime = *(std::localtime(&time));
#endif
	std::stringstream str;
	str << std::put_time(&ltime, "%Y_%m_%d_%H_%M_%S");
	return str.str();
}


uint64_t System::hash64(const void* data, size_t size){
	return XXH3_64bits(data, size);
}

uint32_t System::hash32(const void* data, size_t size){
	return XXH32(data, size, 0);
}

#ifdef _WIN32

wchar_t * System::widen(const std::string & str) {
	const int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
	WCHAR * arr	= new WCHAR[size];
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, static_cast<LPWSTR>(arr), size);
	// \warn Will leak on Windows.
	return arr;
}

std::string System::narrow(wchar_t * str) {
	const int size = WideCharToMultiByte(CP_UTF8, 0, str, -1, nullptr, 0, nullptr, nullptr);
	std::string res(size - 1, 0);
	WideCharToMultiByte(CP_UTF8, 0, str, -1, &res[0], size, nullptr, nullptr);
	return res;
}

#else

const char * System::widen(const std::string & str) {
	return str.c_str();
}
std::string System::narrow(char * str) {
	return std::string(str);
}

#endif
