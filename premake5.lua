local sep = "/"
local ext = ""
if os.ishost("windows") then
	sep = "\\"
	ext = ".exe"
end
cwd = os.getcwd()

-- Workspace definition.

workspace("Rendu")
	-- Configurations
	configurations({ "Release", "Dev"})
	location("build")
	targetdir ("build/%{prj.name}/%{cfg.longname}")
	debugdir ("build/%{prj.name}/%{cfg.longname}")
	architecture("x64")

	-- Configuration specific settings.
	filter("configurations:Release")
		defines({ "NDEBUG" })
		optimize("On")

	filter("configurations:Dev")
		defines({ "DEBUG" })
		symbols("On")

	filter({})
	startproject("ALL")


-- Helper functions for the projects.

function CommonSetup()
	-- C++ settings
	language("C++")
	cppdialect("C++11")
	systemversion("latest")
	-- Compiler flags
	filter("toolset:not msc*")
		buildoptions({ "-Wall", "-Wextra" })
	filter("toolset:msc*")
		buildoptions({ "-W3"})
	filter({})
	-- Common include dirs
	-- System headers are used to support angled brackets in Xcode.
	sysincludedirs({ "src/libs/", "src/libs/glfw/include/" })
end	

function ExecutableSetup()
	kind("ConsoleApp")

	-- Link with compiled librarires
	includedirs({ "src/engine" })
	links({"Engine"})
	links({"nfd", "glfw3"})

	-- Libraries for each platform.
	filter("system:macosx")
		links({"OpenGL.framework", "Cocoa.framework", "IOKit.framework", "CoreVideo.framework", "AppKit.framework"})

	filter("system:windows")
		links({"opengl32", "comctl32"})

	filter("system:linux")
		links({"GL", "X11", "Xi", "Xrandr", "Xxf86vm", "Xinerama", "Xcursor", "Xext", "Xrender", "Xfixes", "xcb", "Xau", "Xdmcp", "rt", "m", "pthread", "dl", "gtk-3", "gdk-3", "pangocairo-1.0", "pango-1.0", "atk-1.0", "cairo-gobject", "cairo", "gdk_pixbuf-2.0", "gio-2.0", "gobject-2.0", "glib-2.0"})
	
	filter({})

end

function ShaderValidation()
	-- Run the shader validator on all existing shaders.
	-- Output IDE compatible error messages.
	dependson({"ShaderValidator"})

	filter("configurations:Release")
		prebuildcommands({ 
			path.translate("{CHDIR} "..cwd.."/build/ShaderValidator/Release/", sep),
			path.translate("./ShaderValidator"..ext.." "..cwd.."/resources/", sep)
		})
	filter("configurations:Dev")
		prebuildcommands({ 
			path.translate("{CHDIR} "..cwd.."/build/ShaderValidator/Dev/", sep),
			path.translate("./ShaderValidator"..ext.." "..cwd.."/resources/", sep)
		})
	filter({})

end	

function RegisterSourcesAndShaders(srcPath, shdPath)
	files({ srcPath, shdPath })
	removefiles({"**.DS_STORE", "**.thumbs"})
	-- Reorganize file hierarchy in the IDE project.
	vpaths({
	   ["*"] = {srcPath},
	   ["Shaders/*"] = {shdPath}
	})
end

function AppSetup(appName)
	CommonSetup()
	ExecutableSetup()
	ShaderValidation()
	-- Declare src and resources files.
	srcPath = "src/apps/"..appName.."/**"
	rscPath = "resources/"..appName.."/shaders/**"
	RegisterSourcesAndShaders(srcPath, rscPath)
end	

function ToolSetup()
	CommonSetup()
	ExecutableSetup()
	ShaderValidation()
end	

-- Projects

project("Engine")
	CommonSetup()
	kind("StaticLib")

	includedirs({ "src/engine" })
	-- Some additional files (README, scenes) are hidden, but you can display them in the project by uncommenting them below.
	files({ "src/engine/**.hpp", "src/engine/**.cpp",
			"resources/common/shaders/**",
			"src/libs/**.hpp", "src/libs/*/*.cpp", "src/libs/**.h",
			"premake5.lua", 
			"README.md",
	--		"resources/**.scene"
	})
	removefiles { "src/libs/nfd/*" }
	removefiles { "src/libs/glfw/*" }
	removefiles({"**.DS_STORE", "**.thumbs"})
	-- Virtual path allow us to get rid of the on-disk hierarchy.
	vpaths({
	   ["Engine/*"] = {"src/engine/**"},
	   ["Shaders/*"] = {"resources/common/shaders/**"},
	   ["Libraries/*"] = {"src/libs/**"},
	   [""] = { "*.*" },
	-- ["Scenes/*"] = {"resources/**.scene"},
	})


group("Apps")

project("PBRDemo")
	AppSetup("pbrdemo")
	
project("Playground")
	AppSetup("playground")

project("Atmosphere")
	AppSetup("atmosphere")

project("SnakeGame")
	AppSetup("snakegame")

project("PathTracer")
	AppSetup("pathtracer")

project("ImageFiltering")
	AppSetup("imagefiltering")


group("Tools")

project("AtmosphericScatteringEstimator")
	ToolSetup()
	files({ "src/tools/AtmosphericScatteringEstimator.cpp" })

project("BRDFEstimator")
	ToolSetup()
	includedirs({ "src/apps/pbrdemo" })
	files({ "src/tools/BRDFEstimator.cpp" })

project("ControllerTest")
	ToolSetup()
	files({ "src/tools/ControllerTest.cpp" })

project("ImageViewer")
	ToolSetup()
	RegisterSourcesAndShaders("src/tools/ImageViewer.cpp", "resources/imageviewer/shaders/**")

project("ObjToScene")
	ToolSetup()
	files({ "src/tools/objtoscene/*.cpp", "src/tools/objtoscene/*.hpp" })

project("ShaderValidator")
	CommonSetup()
	ExecutableSetup()
	files({ "src/tools/ShaderValidator.cpp" })

group("Meta")

project("ALL")
	CommonSetup()
	kind("ConsoleApp")
	dependson( {"Engine", "PBRDemo", "Playground", "Atmosphere", "ImageViewer", "ImageFiltering", "AtmosphericScatteringEstimator", "BRDFEstimator", "ControllerTest", "SnakeGame", "PathTracer", "ObjToScene"})

-- Include NFD premake file.

include("src/libs/nfd/premake5.lua")
include("src/libs/glfw/premake5.lua")

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

newaction {
   trigger     = "docs",
   description = "Build the documentation using Doxygen",
   execute     = function ()
      print("Generating documentation...")
      os.execute("doxygen"..ext.." docs/Doxyfile")
      print("Done.")
   end
}

-- Internal private projects can be added here.
if os.isfile("src/internal/premake5.lua") then
	include("src/internal/premake5.lua")
end
	
