#Compiler: g++
CXX = g++

#Include directories (for headers): standard include dirs in /usr and /usr/local, and our helper directory.
INCLUDEDIR = -I/usr/include/ -I/usr/local/include/ -Isrc/helpers/ -Isrc/libs/ -Isrc/libs/glfw/include/

LIBDIR = -Lsrc/libs/glfw/lib-mac/
#Libraries needed: OpenGL and glfw3. glfw3 requires Cocoa, IOKit and CoreVideo.
LIBS = $(LIBDIR) -lglfw3 -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo

#Compiler flags: C++11 standard, and display 'all' warnings.
CXXFLAGS = -std=c++11 -Wall -O3

#Build directory
BUILDDIR = build
#Source directory
SRCDIR = src

#Paths to the source files
#Collect sources from subdirectories (up to depth 3) 
# /!\ at depth 4, src/libs/glm/detail contains dummy cpp files that we want to ignore.
SRC_DEPTH_1 = $(wildcard $(SRCDIR)/*.cpp)
SRC_DEPTH_2 = $(wildcard $(SRCDIR)/*/*.cpp)
SRC_DEPTH_3 = $(wildcard $(SRCDIR)/*/*/*.cpp)
SOURCES = $(SRC_DEPTH_3) $(SRC_DEPTH_2) $(SRC_DEPTH_1)
# Filtered sources: tools on a side, other files on the other.
SOURCES_TOOLS = $(wildcard $(SRCDIR)/tools/*.cpp)
SOURCES_FILTERED = $(filter-out src/main.cpp, $(filter-out $(SOURCES_TOOLS), $(SOURCES)))

#Paths to the object files
OBJECTS_FILTERED = $(SOURCES_FILTERED:$(SRCDIR)/%.cpp=$(BUILDDIR)/%.o)
TOOLS_FILTERED = $(SOURCES_TOOLS:$(SRCDIR)/tools/%.cpp=tools/%)

#Paths to the subdirectories
SUBDIRS_LIST = $(shell find src -type d)
SUBDIRS = $(SUBDIRS_LIST:$(SRCDIR)%=$(BUILDDIR)%)

#Executable name
EXECNAME = glprogram

#Re-create the build dir if needed, compile and link.
all: engine tools
engine: dirs $(EXECNAME)
tools: dirs $(TOOLS_FILTERED)

#Linking phase: combine all objects files to generate the executable
$(EXECNAME): $(BUILDDIR)/main.o $(OBJECTS_FILTERED) 
	@echo "Linking $(EXECNAME)..."
	@$(CXX) $(LIBDIR) $(OBJECTS_FILTERED) $< $(LIBS) -o $(BUILDDIR)/$@
	@echo "Done!"

tools/% : $(BUILDDIR)/tools/%.o $(OBJECTS_FILTERED) 
	@echo "Linking $@..."
	@$(CXX) $(LIBDIR) $(OBJECTS_FILTERED) $< $(LIBS) -o $(BUILDDIR)/$@
	@echo "Done!"

#Compiling phase: generate the object files from the source files
$(BUILDDIR)/%.o : $(SRCDIR)/%.cpp
	@echo "Compiling $<"
	@$(CXX) -c $(CXXFLAGS) $(INCLUDEDIR)  $< -o $@

#Run the executable
run:
	@./$(BUILDDIR)/$(EXECNAME)

run/%:
	@./$(BUILDDIR)/tools/$*

#Create the build directory and its subdirectories
dirs:
	@mkdir -p $(SUBDIRS)

#Remove the whole build directory
.PHONY: clean

#Disable automatic temporary files deletion.
.SECONDARY:

clean :
	rm -r $(BUILDDIR)

