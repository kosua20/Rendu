local sep = "/"
local ext = ""
-- On Windows we have to use specific separator and executable extension.
if os.ishost("windows") then
	sep = "\\"
	ext = ".exe"
end
-- On Linux We have to query the dependencies of gtk+3 for NFD, and convert them to a list of libraries, we do this on the host for now.
if os.ishost("linux") then
	gtkList, code = os.outputof("pkg-config --libs gtk+-3.0")
	gtkLibs = string.explode(string.gsub(gtkList, "-l", ""), " ")
end

cwd = os.getcwd()
projects = {}

-- Options
newoption {
	 trigger     = "skip_shader_validation",
	 description = "Do not validate shaders at application/tool compilation."
}

newoption {
	 trigger     = "skip_internal",
	 description = "Do not generate any existing internal projects."
}

-- Workspace definition.

workspace("Rendu")
	-- Configurations
	configurations({ "Release", "Dev"})
	location("build")
	targetdir ("build/%{prj.name}/%{cfg.longname}")
	debugdir ("build/%{prj.name}/%{cfg.longname}")
	architecture("x64")
	systemversion("latest")

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
	-- Compiler flags
	filter("toolset:not msc*")
		buildoptions({ "-Wall", "-Wextra" })
	filter("toolset:msc*")
		buildoptions({ "-W3"})
	filter({})
	-- Common include dirs
	-- System headers are used to support angled brackets in Xcode.
	sysincludedirs({ "src/libs/", "src/libs/glfw/include/" })
	
	-- System headers are used to support angled brackets in Xcode.
	filter("system:macosx" or "system:linux")
		sysincludedirs({ "/usr/local/include/" })
		libdirs({ "/usr/local/lib" })

	filter("system:windows")
		sysincludedirs({ "$(VULKAN_SDK)/include"})
		libdirs({ "$(VULKAN_SDK)/lib" })
	filter({})
end	

function ExecutableSetup()
	kind("ConsoleApp")
	CommonSetup()

	-- Link with compiled librarires
	includedirs({ "src/engine" })
	links({"Engine"})

	links({"nfd", "glfw3"})
	
	-- Libraries for each platform.
	filter("system:macosx")
		links({"Cocoa.framework", "IOKit.framework", "CoreVideo.framework", "AppKit.framework", "pthread"})
		
	filter("system:linux")
		-- We have to query the dependencies of gtk+3 for NFD, and convert them to a list of libraries.
		links({"X11", "Xi", "Xrandr", "Xxf86vm", "Xinerama", "Xcursor", "Xext", "Xrender", "Xfixes", "xcb", "Xau", "Xdmcp", "rt", "m", "pthread", "dl", gtkLibs})
	
	filter("system:windows")
		links({"comctl32"})

	-- Vulkan dependencies
	filter("system:macosx" or "system:linux")
		links({"glslang", "OSDependent", "MachineIndependent", "GenericCodeGen", "OGLCompiler", "SPIRV", "SPIRV-Tools", "SPIRV-Tools-opt" })
	
	filter({"system:windows", "configurations:Dev"})
		links({"glslangd", "OSDependentd", "MachineIndependentd", "GenericCodeGend", "OGLCompilerd", "SPIRVd", "SPIRV-Toolsd", "SPIRV-Tools-optd" })
	
	filter({"system:windows", "configurations:Release" })
		links({"glslang", "OSDependent", "MachineIndependent", "GenericCodeGen", "OGLCompiler", "SPIRV", "SPIRV-Tools", "SPIRV-Tools-opt" })

	filter({})

	-- Register in the projects list for the ALL target.
	table.insert(projects, project().name)

end

function ShaderValidation()
	if _OPTIONS["skip_shader_validation"] then
		return
	end
	-- Run the shader validator on all existing shaders.
	-- Output IDE compatible error messages.
	dependson({"ShaderValidator"})
	
	filter("configurations:*")
		postbuildcommands({
			path.translate(cwd.."/build/ShaderValidator/%{cfg.longname}/ShaderValidator"..ext.." "..cwd.."/resources/", sep)
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
	ExecutableSetup()
	ShaderValidation()
	-- Declare src and resources files.
	RegisterSourcesAndShaders("src/apps/"..appName.."/**", "resources/"..appName.."/shaders/**")
end	

-- Projects

project("Engine")
	CommonSetup()
	kind("StaticLib")

	includedirs({ "src/engine" })
	-- Some additional files (README, scenes) are hidden, but you can display them in the project by uncommenting them below.
	files({ "src/engine/**.hpp", "src/engine/**.cpp",
			"resources/common/shaders/**",
			"src/libs/**.hpp", "src/libs/*/*.cpp", "src/libs/**.h", "src/libs/*/*.c",
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

	filter("system:macosx")
		defines({"VK_USE_PLATFORM_MACOS_MVK"})

	filter("system:windows")
		defines({"VK_USE_PLATFORM_WIN32_KHR"})

	filter("system:linux")
		defines({"VK_USE_PLATFORM_XLIB_KHR"})
	



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

project("ShaderBench")
	AppSetup("shaderbench")

project("Island")
	AppSetup("island")

project("Stenciled")
	AppSetup("stenciled")


group("Tools")

project("BRDFEstimator")
	ExecutableSetup()
	ShaderValidation()
	files({ "src/tools/BRDFEstimator.cpp" })

project("ControllerTest")
	ExecutableSetup()
	files({ "src/tools/ControllerTest.cpp" })

project("ImageViewer")
	ExecutableSetup()
	ShaderValidation()
	RegisterSourcesAndShaders("src/tools/ImageViewer.cpp", "resources/imageviewer/shaders/**")

project("ObjToScene")
	ExecutableSetup()
	files({ "src/tools/objtoscene/*.cpp", "src/tools/objtoscene/*.hpp" })

project("SceneEditor")
	ExecutableSetup()
	ShaderValidation()
	files({ "src/tools/sceneeditor/*.cpp", "src/tools/sceneeditor/*.hpp" })

project("ShaderValidator")
	ExecutableSetup()
	files({ "src/tools/ShaderValidator.cpp" })
	

group("Meta")

project("ALL")
	kind("ConsoleApp")
	CommonSetup()
	dependson({ "Engine" })
	dependson( projects )
	-- We need a dummy file to execute.
	files({ "src/tools/ALL.cpp" })

project("DOCS")
	kind("ConsoleApp")
	prebuildcommands({ 
		path.translate("cd "..cwd),
		path.translate("doxygen"..ext.." docs/Doxyfile")
	})
	-- We need a dummy file to execute.
	files({ "src/tools/ALL.cpp" })


group("Dependencies")

-- Include NFD and GLFW premake files.
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

newaction {
   trigger     = "list",
   description = "List projects that will be built in the 'ALL' project",
   execute     = function ()
   		print("Found "..#projects.." projects:")
      	for i,v in ipairs(projects) do print(" * "..v) end
   end
}

-- Internal private projects can be added here.
if (not _OPTIONS["skip_internal"]) and os.isfile("src/internal/premake5.lua") then
	include("src/internal/premake5.lua")
end
	
