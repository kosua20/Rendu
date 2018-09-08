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
doxyNamespace = "GLSL"
doxyGroup = "Shaders"
doxySubnamespace = fileExt.lower()[1:].capitalize()
doxyClass = fileName.capitalize().replace("-", "_")

# C++ namespaces
printout("namespace " + doxyNamespace + "{\n")
printout("namespace " + doxySubnamespace + "{\n")
# Class
printout("/** \\class " + doxyClass + "\n")
printout("  * \\ingroup " + doxyGroup + "*/\n")
printout("public class " + doxyClass + " {" + "\n")
printout("public:" + "\n")
# Content of the shader
for line in fileLines:
	printout(line)
# Close the class, the namespaces, the group.
printout("}" + "\n}\n}\n")

sys.exit(0)