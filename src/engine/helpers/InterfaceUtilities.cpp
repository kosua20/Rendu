#include "InterfaceUtilities.hpp"
#include "../Common.hpp"
#include <nfd/nfd.h>


namespace Interface {
	
	void setup(GLFWwindow * window){
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		ImGui_ImplGlfw_InitForOpenGL(window, false);
		ImGui_ImplOpenGL3_Init("#version 150");
		ImGui::StyleColorsDark();
	}
		
	void beginFrame(){
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}
	
	void endFrame(){
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}
	
	void clean(){
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
	
	bool showPicker(const PickerMode mode, const std::string & startPath, std::string & outPath, const std::string & extensions){
		nfdchar_t *outPathRaw = NULL;
		nfdresult_t result = NFD_CANCEL;
		outPath = "";
		
		if(mode == Load){
			result = NFD_OpenDialog(extensions.empty() ? NULL : extensions.c_str(), startPath.c_str(), &outPathRaw);
		} else if(mode == Save){
			result = NFD_SaveDialog(extensions.empty() ? NULL : extensions.c_str(), startPath.c_str(), &outPathRaw);
		} else if(mode == Directory){
			result = NFD_PickFolder(startPath.c_str(), &outPathRaw);
		}
		
		if (result == NFD_OKAY) {
			outPath = std::string(outPathRaw);
			free(outPathRaw);
			return true;
		} else if (result == NFD_CANCEL) {
			// Cancelled by user, nothing to do.
		} else {
			// Real error.
			Log::Error() << "Unable to present system picker (" <<  std::string(NFD_GetError()) << ")." << std::endl;
		}
		free(outPathRaw);
		return false;
	}
}








