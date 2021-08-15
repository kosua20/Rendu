#!/usr/bin/env python
# -*- coding: utf-8 -*- 

# If no documentation is generated, make sure that this file is executable.
import sys
import os

def printout(text):
	sys.stdout.write(text)

doxyFullNames = {"vert" : "vertex", "frag" : "fragment", "geom" : "geometry", "glsl" : "general", "tessc" : "tesselation control", "tesse" : "tesselation evaluation" }
doxySubNamespaces = {"vert" : "Vert", "frag" : "Frag", "geom" : "Geom", "glsl" : "Common", "tesse" : "TessEval", "tessc" : "TessControl" }
structAddKeyword = {"vert" : "out", "frag" : "in", "geom" : "in", "glsl" : "", "tessc" : "in", "tesse" : "out" }

if len(sys.argv) != 2:
	sys.exit(1)

# Get the file infos.
filePath = sys.argv[1]
fileSubpath, fileExt = os.path.splitext(filePath)
fileDir, fileName = os.path.split(fileSubpath)
fileExt = fileExt
fileName = fileName
fileType = fileExt.lower()[1:]

# Get the file content.
fileHandle = open(filePath)
fileLines = fileHandle.readlines()
fileHandle.close()

# Doxygen infos.
doxyNamespace = "GPUShaders"
doxyGroup = "Shaders"

subNamespace = doxySubNamespaces[fileType]
descripName = doxyFullNames[fileType]
structKeyword = structAddKeyword[fileType]

className = fileName.capitalize().replace("-", "_")
classDetails = className.replace("_", " ")

# Start by parsing the body of the shader and wrapping it into a C++ class.
# We adjust include, layout and blocks keywords to avoid issues with Doxygen.
# We do this first so that we can reference included shaders afterwards.
bodyStr  = "public class " + className + " {" + "\n"
bodyStr += "public:" + "\n"

inInterfaceBlock = False
inUniformBlock = False
referencedFiles = []
# Content of the shader
for line in fileLines:
	printLine = True
	# Detect layout keyword at beginning and remove layout block that make Doxygen think it's a function.
	if line.lstrip().startswith("layout"):
		line = line[line.find(")")+1:].lstrip()
	if line.find("INTERFACE")>=0:
		printLine = False
		inInterfaceBlock = True
	if line.find("uniform")>=0 and line.find("sampler")<0:
		printLine = False
		inUniformBlock = True
	if (inInterfaceBlock or inUniformBlock) and line.find("}") >= 0:
		printLine = False
		inInterfaceBlock = False
		inUniformBlock = False
	if inInterfaceBlock and len(line) > 0:
		line = structKeyword + " " + line.lstrip()
	if inUniformBlock and len(line) > 0:
		line = "uniform" + " " + line.lstrip()
	# Skip our include system directives, but keep track 
	# of them so that we can reference them in the description.
	incPos = line.find("#include")
	if incPos>=0:
		nameStart = line.find("\"", incPos)
		nameEnd = line.find(".glsl\"", nameStart)
		name = line[(nameStart+1):(nameEnd)]
		referencedFiles += [name]
		printLine = False
	if printLine:
		bodyStr += line

# Close the class.
bodyStr += "\n};"


# C++ namespaces
headerStr  = "namespace " + doxyNamespace + " {\n"
headerStr += "namespace " + subNamespace + " {\n"
# Class description
headerStr += "/** \\class " + className + "\n"
headerStr += "  * \\brief " + classDetails + " " + descripName + " shader." + "\n"
# Reference included shaders
if(len(referencedFiles) > 0):
	headerStr += "  * \\sa "
	firstSA  = True
	for refFile in referencedFiles:
		if not firstSA:
			headerStr += ", "
		else:
			firstSA = False

		refName = refFile.capitalize().replace("-", "_")
		headerStr += doxyNamespace + "::" + doxySubNamespaces["glsl"] + "::" + refName
	headerStr += "\n"
headerStr += "  * \\ingroup " + doxyGroup + "\n*/\n"

# Close the namespaces
bodyStr += "\n}\n}\n"

printout(headerStr)
printout(bodyStr)

sys.exit(0)
