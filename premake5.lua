-- Workspace definition.

workspace("GL_Template")
	-- Configurations
	configurations({ "Debug", "Release"})
	location("build")
	targetdir ("build/%{prj.name}/%{cfg.longname}")

	-- Configuration specific settings.
	filter("configurations:Debug")
		defines({ "DEBUG" })
		symbols("On")
	filter("configurations:Release")
		defines({ "NDEBUG" })
		optimize("On")
	filter({})


-- Helper functions for the projects.

function CPPSetup()
	language("C++")
	buildoptions({ "-std=c++11","-Wall" })
end	

function GraphicsSetup()
	CPPSetup()

	-- To support angled brackets in Xcode.
	sysincludedirs({ "src/libs/", "src/libs/glfw/include/" })

	-- Libraries for each platform.
	if os.istarget("macosx") then
		libdirs({"src/libs/glfw/lib-mac/"})
		links({"glfw3", "OpenGL.framework", "Cocoa.framework", "IOKit.framework", "CoreVideo.framework"})
	elseif os.istarget("windows") then
		libdirs({"src/libs/glfw/lib-win-vc2015/"})
		links({"glfw3", "opengl32"})
		-- Might also need : kernel32.lib;user32.lib;gdi32.lib;winspool.lib;comdlg32.lib;advapi32.lib;shell32.lib;ole32.lib;oleaut32.lib;uuid.lib;odbc32.lib;odbccp32.lib;
	else 
		-- Assume linux
		-- Libraries needed: OpenGL and glfw3.  glfw3 require X11, Xi, and so on...	
		libdirs({ os.findlib("glfw3") })
		links({"glfw3", "GL", "X11", "Xi", "Xrandr", "Xxf86vm", "Xinerama", "Xcursor", "rt", "m", "pthread", "dl"})
	end

end

function AppSetup()
	GraphicsSetup()
	includedirs({ "src/engine" })
	links({"Engine"})
	kind("ConsoleApp")
end	

function ToolSetup()
	AppSetup()
	location("build/tools")
end	


-- Projects

project("Engine")
	GraphicsSetup()
	kind("StaticLib")
	files({ "src/engine/**.hpp", "src/engine/**.cpp",
			"resources/shaders/**.vert", "resources/shaders/**.frag",
			"src/libs/*/*.hpp", "src/libs/*/*.cpp", "src/libs/*/*.h"
	})


group("Apps")

project("PBRDemo")
	AppSetup()
	files({ "src/apps/gltemplate/**.hpp", "src/apps/gltemplate/**.cpp", })


group("Tools")

project("BRDFEstimator")
	ToolSetup()
	includedirs({ "src/apps/gltemplate" })
	files({ "src/tools/BRDFEstimator.cpp" })

project("SHExtractor")
	ToolSetup()	
	files({ "src/tools/SHExtractor.cpp" })


-- Actions

newaction {
   trigger     = "clean",
   description = "Clean the build directory",
   execute     = function ()
      print("Cleaning...")
      os.rmdir("./build")
      print("Done.")
   end
}

-- TODO: package resources automatically.
	