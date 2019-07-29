
project("glfw3")
	kind("StaticLib")

	language("C")
	systemversion("latest")

	-- common files
	files({"src/internal.h", "src/mappings.h", "src/glfw_config.h",
			"include/GLFW/glfw3.h", "include/GLFW/glfw3native.h", 
		"src/context.c", "src/init.c", "src/input.c", "src/monitor.c", "src/vulkan.c", "src/window.c", 
		"premake5.lua"})
	
	-- system build filters
	filter("system:windows")
		defines({"_GLFW_WIN32=1"})
		files({"src/win32_platform.h","src/win32_joystick.h","src/wgl_context.h","src/egl_context.h",
			"src/osmesa_context.h","src/win32_init.c","src/win32_joystick.c","src/win32_monitor.c","src/win32_time.c",
			"src/win32_thread.c","src/win32_window.c","src/wgl_context.c","src/egl_context.c","src/osmesa_context.c"})

	filter("system:macosx")
		defines({"_GLFW_COCOA=1"})
		files({"src/cocoa_platform.h","src/cocoa_joystick.h","src/posix_thread.h","src/nsgl_context.h",
		"src/egl_context.h","src/osmesa_context.h","src/cocoa_init.m","src/cocoa_joystick.m","src/cocoa_monitor.m",
		"src/cocoa_window.m","src/cocoa_time.c","src/posix_thread.c","src/nsgl_context.m","src/egl_context.c",
		"src/osmesa_context.c"})
		
	filter("system:linux")
		defines({"_GLFW_X11=1"})
		files({"src/x11_platform.h", "src/xkb_unicode.h", "src/posix_time.h", "src/posix_thread.h", "src/glx_context.h", 
			"src/egl_context.h", "src/osmesa_context.h", "src/x11_init.c", "src/x11_monitor.c", "src/x11_window.c",
			"src/xkb_unicode.c", "src/posix_time.c", "src/posix_thread.c", "src/glx_context.c", "src/egl_context.c",
			"src/osmesa_context.c", "src/linux_joystick.h", "src/linux_joystick.c"})
	
	-- visual studio filters
	filter("action:vs*")
		defines({ "_CRT_SECURE_NO_WARNINGS" })    

	filter({})

