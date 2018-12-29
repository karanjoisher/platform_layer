import sys
import os
import time

def Index(arr, elem):
    try:
        index_value = arr.index(elem)
    except ValueError:
        index_value = -1
    return index_value

def EatWhiteSpace(line):
    endIndex = len(line) - 1
    for i in range(0, len(line)):
        if(not line[i].isspace()):
            endIndex = i
            break

    return line[endIndex:]

def EatTrailingWhiteSpace(line):
    reverseLine = line[::-1]
    reverseLine = EatWhiteSpace(reverseLine)
    line = reverseLine[::-1]
    return line

def EatWhiteSpaceFromBothEnds(line):
    line = EatWhiteSpace(line)
    line = EatTrailingWhiteSpace(line)
    return line

def RemoveWord(line, word):

    index = Index(line, word)
    while(index >= 0):
        line = line[0: index] + line[index + len(word) : ] 
        index = Index(line, word)
    return line

def ReplaceWord(line, oldWord, newWord):
    index = Index(line, oldWord)
    while(index >= 0):
        line = line[0: index] + newWord + line[index + len(oldWord) : ] 
        index = Index(line, oldWord)
    return line
 
    

loadedFunctions = []


if __name__ == "__main__":

    start = time.time()

    filepaths = []

    platform = sys.argv[1]

    # Adding all files to look for pending opengl functions
    for i in range(2, len(sys.argv)):

        if(os.path.isdir(sys.argv[i])):
            dirPath = os.path.abspath(sys.argv[i])
            for filename in os.listdir(sys.argv[i]):
                if filename[-3:] == "cpp":
                    filepaths.append(dirPath + "/" + filename)
        else:
            filepaths.append(os.path.abspath(sys.argv[i]))


    if platform == "PLATFORM_WINDOWS":
        openglFilePath = "windows_opengl.h"
    elif platform == "PLATFORM_LINUX":
        openglFilePath = "linux_opengl.h"

    glFuncPrototypeFilePath = "glcorearb.h"

    openglFile = open(openglFilePath, "r")
    glFuncPrototypeFile = open(glFuncPrototypeFilePath, "r")

    # Get opengl functions that are already defined
    for line in openglFile:
        line = EatWhiteSpace(line)
        toMatch = "GL_GET_PROC_ADDRESS(";
        if len(line) >= len(toMatch) and line[0: len(toMatch)] == toMatch:
            functionNameStartIndex = len(toMatch)
            functionNameOnePastEndIndex = line.index(")")
            functionNameEndIndex = functionNameOnePastEndIndex - 1
            functionName = line[functionNameStartIndex : functionNameEndIndex + 1]
            functionName = EatWhiteSpaceFromBothEnds(functionName)
            loadedFunctions.append(functionName)

    openglFile.close()

    # Get opengl functions that are already defined
    if platform == "PLATFORM_WINDOWS":
        glHeaderFilePath = "C:/Program Files (x86)/Windows Kits/8.1/Include/um/gl/GL.h"
    elif platform == "PLATFORM_LINUX":
        glHeaderFilePath = "/usr/include/GL/gl.h"
        
    glFile = open(glHeaderFilePath, "r")
    for line in glFile:
        line = EatWhiteSpace(line)
        if platform == "PLATFORM_WINDOWS":
            toMatch = "WINGDIAPI"
        elif platform == "PLATFORM_LINUX":
            toMatch = "GLAPI"

        
        if len(line) >= len(toMatch) and line[0: len(toMatch)] == toMatch:
            line = line[len(toMatch):]
            functionNameStartIndex = line.index("gl", len(toMatch))
            functionNameOnePastEndIndex = line.index("(", functionNameStartIndex)
            functionNameEndIndex = functionNameOnePastEndIndex - 1
            functionName = line[functionNameStartIndex : functionNameEndIndex + 1]
            functionName = EatTrailingWhiteSpace(functionName)
            loadedFunctions.append(functionName)
    glFile.close()
    

    # Get opengl functions used in the files given as cmd args
    nameOfFuncsToGen = []
    for path in filepaths:
        file = open(path, "r")

        for line in file:
            line = EatWhiteSpace(line)
            toMatch = "GL_CALL("
            if len(line) >= len(toMatch) and line[0: len(toMatch)] == toMatch:
                functionNameStartIndex = line.index("gl", len(toMatch))
                functionNameOnePastEndIndex = line.index("(", functionNameStartIndex)
                functionNameEndIndex = functionNameOnePastEndIndex - 1
                functionName = line[functionNameStartIndex : functionNameEndIndex + 1]
                functionName = EatTrailingWhiteSpace(functionName)
                if(Index(loadedFunctions, functionName) < 0 and Index(nameOfFuncsToGen, functionName) < 0):
                    nameOfFuncsToGen.append(functionName)
        file.close()


    # Get opengl function prototypes
    glPrototypes = [""] * len(nameOfFuncsToGen)
    for line in glFuncPrototypeFile:
        line = EatWhiteSpace(line)
        toMatch = "GLAPI"
        if len(line) >= len(toMatch) and line[0: len(toMatch)] == toMatch:
            line = line[len(toMatch):]
            functionNameStartIndex = line.index("gl", len(toMatch))
            functionNameOnePastEndIndex = line.index("(", functionNameStartIndex)
            functionNameEndIndex = functionNameOnePastEndIndex - 1
            functionName = line[functionNameStartIndex : functionNameEndIndex + 1]
            functionName = EatTrailingWhiteSpace(functionName)
            
            index = Index(nameOfFuncsToGen, functionName)
            if(index >= 0):
                prototype = line
                prototype = EatWhiteSpaceFromBothEnds(prototype)
                glPrototypes[index] = prototype
                #TODO : del namesOfFuncsToGen[index]
    glFuncPrototypeFile.close()
    

    # Generate C++ code requried for importing opengl functions
    typedefString = ""
    functionDeclarationString = ""
    getProcAddressString = ""
    newFunctionsAdded = 0
    for i in range(0, len(nameOfFuncsToGen)):
        
        if(len(glPrototypes[i]) == 0):
            continue
        
        newFunctionsAdded += 1
        prototype = ReplaceWord(glPrototypes[i], " " + nameOfFuncsToGen[i] + " ", " type_" + nameOfFuncsToGen[i] + " ")
        typedefString += "typedef " + prototype + "\n"

        functionDeclarationString += "GL_FUNCTION(" + nameOfFuncsToGen[i] + ");\n"
        getProcAddressString += "GL_GET_PROC_ADDRESS(" + nameOfFuncsToGen[i] + ");\n"


    # Backing up current opengl file
    openglFile = open(openglFilePath, "r")
    backup = openglFile.read()
    openglFileBackup = open(openglFilePath + "backup", "w")
    openglFileBackup.write(backup)
    openglFileBackup.close()

    openglFile.seek(0)
    newOpenglFileContent = ""

    # Generate updated opengl file
    for line in openglFile:

        tempLine = EatWhiteSpace(line)
        toMatch = "INSERT_NEW_FUNCTION_TYPES_HERE"
        if len(tempLine) >= len(toMatch) and tempLine[0: len(toMatch)] == toMatch:
            newOpenglFileContent += line
            newOpenglFileContent += typedefString
            continue

        toMatch = "INSERT_NEW_FUNCTION_DECLARATIONS_HERE"
        if len(tempLine) >= len(toMatch) and tempLine[0: len(toMatch)] == toMatch:
            newOpenglFileContent += line
            newOpenglFileContent += functionDeclarationString
            continue

        toMatch = "INSERT_NEW_FUNCTION_GRABS_HERE"
        if len(tempLine) >= len(toMatch) and tempLine[0: len(toMatch)] == toMatch:
            newOpenglFileContent += line
            newOpenglFileContent += getProcAddressString
            continue

        newOpenglFileContent += line



    openglFile.close()
    openglFile = open(openglFilePath, "w")
    openglFile.write(newOpenglFileContent)
    openglFile.close()

    end = time.time()
    t = end - start
    print("Generated " + str(newFunctionsAdded) + " opengl function(s) in " + str(t) + " seconds\n")
