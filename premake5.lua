workspace("GL_Template")
	configurations({ "Debug", "Release"})
	location("build")

	language("C++")
	buildoptions({ "-std=c++11","-Wall" })

	-- To support angled brackets in Xcode.
	sysincludedirs({ "src/libs/", "src/libs/glfw/include/" })

	if os.istarget("macosx") then
		libdirs({"src/libs/glfw/lib-mac/"})
		links({"glfw3", "OpenGL.framework", "Cocoa.framework", "IOKit.framework", "CoreVideo.framework"})
	elseif os.istarget("windows") then
		libdirs({"src/libs/glfw/lib-win-vc2015/"})
		links({"glfw3", "opengl32"})
		--kernel32.lib;user32.lib;gdi32.lib;winspool.lib;comdlg32.lib;advapi32.lib;shell32.lib;ole32.lib;oleaut32.lib;uuid.lib;odbc32.lib;odbccp32.lib;
	else -- assume linux
		-- Libraries needed: OpenGL and glfw3.  glfw3 require X11, Xi, and so on...	
		libdirs({ os.findlib("glfw3") })
		links({"glfw3", "GL", "X11", "Xi", "Xrandr", "Xxf86vm", "Xinerama", "Xcursor", "rt", "m", "pthread", "dl"})
	end

	filter("configurations:Debug")
		defines({ "DEBUG" })
		symbols("On")
	filter("configurations:Release")
		defines({ "NDEBUG" })
		optimize("On")
	filter({})

	targetdir ("build/%{prj.name}/%{cfg.longname}")

-- Projects

project("Engine")
	kind("StaticLib")
	files({ "src/engine/**.hpp", "src/engine/**.cpp",
			"resources/shaders/**.vert", "resources/shaders/**.frag",
			"src/libs/*/*.hpp", "src/libs/*/*.cpp", "src/libs/*/*.h", "src/libs/*/*.c"
	})

project("PBRDemo")
	kind("ConsoleApp")
	includedirs({ "src/engine" })
	links({"Engine"})
	files({ "src/apps/gltemplate/**.hpp", "src/apps/gltemplate/**.cpp", })

project("BRDFEstimator")
	kind("ConsoleApp")
	location("build/tools")
	includedirs({ "src/engine", "src/apps/gltemplate" })
	links({"Engine"})
	files({ "src/tools/BRDFEstimator.cpp" })

project("SHExtractor")
	kind("ConsoleApp")
	location("build/tools")
	includedirs({ "src/engine" })
	links({"Engine"})
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
	