-- Native file dialog premake5 script

newoption {
	 trigger     = "nfd_linux_backend",
	 value       = "B",
	 description = "Choose a dialog backend for NFD on linux",
	 allowed = {
			{ "gtk3", "GTK 3 - link to gtk3 directly" },      
			{ "zenity", "Zenity - generate dialogs on the end users machine with zenity" }
	 }
}

if not _OPTIONS["nfd_linux_backend"] then
	 _OPTIONS["nfd_linux_backend"] = "gtk3"
end


project("nfd")
	kind("StaticLib")

	-- common files
	files({"*.h", "nfd_common.c" })

	-- system build filters
	filter("system:windows")
		language("C++")
		files({"nfd_win.cpp"})

	filter({"action:gmake or action:xcode4"})
		buildoptions({"-fno-exceptions"})

	filter("system:macosx")
		language("C")
		files({"nfd_cocoa.m"})

	filter({"system:linux", "options:nfd_linux_backend=gtk3"})
		language("C")
		files({"nfd_gtk.c"})
		buildoptions({"`pkg-config --cflags gtk+-3.0`"})

	filter({"system:linux", "options:nfd_linux_backend=zenity"})
		language("C")
		files({"nfd_zenity.c"})

	-- visual studio filters
	filter("action:vs*")
		defines({ "_CRT_SECURE_NO_WARNINGS" })    

	filter({})

