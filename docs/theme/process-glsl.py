#!/usr/bin/env python
# -*- coding: utf-8 -*- 

# If no documentation is generated, make sure that this file is executable.
import sys
import os

def printout(text):
	sys.stdout.write(text)

if len(sys.argv) != 2:
	sys.exit(1)

# Get the file infos.
filePath = sys.argv[1]
fileSubpath, fileExt = os.path.splitext(filePath)
fileDir, fileName = os.path.split(fileSubpath)
fileExt = fileExt
fileName = fileName

# Get the file content.
fileHandle = open(filePath)
fileLines = fileHandle.readlines()
fileHandle.close()

# Doxygen infos.
doxyNamespace = "GPU"
doxyGroup = "Shaders"
doxySubnamespace = fileExt.lower()[1:].capitalize()
doxyClass = fileName.capitalize().replace("-", "_")
doxyFullNames = {"Vert" : "vertex", "Frag" : "fragment", "Geom" : "geometry" }
structAddKeyword = {"Vert" : "out", "Frag" : "in", "Geom" : "in" }
doxyClassDetail = doxyClass.replace("_", " ")
structKeyword = structAddKeyword[doxySubnamespace]
# C++ namespaces
printout("namespace " + doxyNamespace + "{\n")
printout("namespace " + doxySubnamespace + "{\n")
# Class
printout("/** \\class " + doxyClass + "\n")
printout("  * \\brief \"" + doxyClassDetail + "\" " + doxyFullNames[doxySubnamespace] + " shader." + "\n")
printout("  * \\ingroup " + doxyGroup + "\n*/\n")
printout("public class " + doxyClass + " {" + "\n")
printout("public:" + "\n")

inInterfaceBlock = False
# Content of the shader
for line in fileLines:
	printLine = True
	# Detect layout keyword at beginning and remove layout block that make Doxygen think it's a function.
	if line.lstrip().startswith("layout"):
		line = line[line.find(")")+1:].lstrip()
	if line.find("INTERFACE")>=0:
		printLine = False
		inInterfaceBlock = True
	if inInterfaceBlock and line.find("}") >= 0:
		printLine = False
		inInterfaceBlock = False
	if inInterfaceBlock and len(line) > 0:
		line = structKeyword + " " + line.lstrip()
	
	if printLine:
		printout(line)
# Close the class, the namespaces, the group.
printout("\n}}}\n")

sys.exit(0)
