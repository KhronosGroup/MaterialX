#!/usr/bin/env python
'''
MaterialX glTF bi-directional conversion example
'''

import sys, os, io, argparse
import MaterialX as mx
import MaterialX.PyMaterialXRender as mx_gltf

def skipLibraryElement(elem):
    return not elem.hasSourceUri()

def main():
    parser = argparse.ArgumentParser(description="MaterialX glTF converter")
    parser.add_argument(dest="inputFilename", help="Filename of the input document (glTF or MaterialX).")
    opts = parser.parse_args()

    inputFilename = opts.inputFilename
    if not os.path.isfile(inputFilename):
        print('Input file not found: ' + inputFilename)
        return

    inputFilePath = mx.FilePath(inputFilename)
    
    toGLTF = inputFilePath.getExtension() == 'mtlx'
    toMaterialX = inputFilePath.getExtension() == 'gltf'

    if not toGLTF and not toMaterialX:
        print('Input file must be glTF or MaterialX')
        return

    handler = mx_gltf.GltfMaterialHandler()
    if not handler:
        print('Failed to create GltfMaterialHandler')
        return
    
    log = []

    stdlib = mx.createDocument()
    searchPath = mx.getDefaultDataSearchPath()
    libraryFolders = []
    libraryFolders.extend(mx.getDefaultDataLibraryFolders())
    try:
        mx.loadLibraries(libraryFolders, searchPath, stdlib)
    except mx.Exception as err:
        print('Failed to load standard libraries: "', err, '"')
        sys.exit(-1)

    # Convert from Materialx to GLTF
    if toGLTF:
        doc = mx.createDocument()
        mx.readFromXmlFile(doc, inputFilePath)
        doc.importLibrary(stdlib)

        outputFilePath = inputFilePath
        print('Converting MaterialX file:%s to glTF file: %s' % 
              (inputFilePath.asString(), outputFilePath.asString() + '.gltf'))
        handler.setMaterials(doc)
        convert = handler.save(outputFilePath.asString() + '.gltf', log)
        print('- Converted: ' + str(convert))

    # Convert from GLTF to MaterialX
    elif toMaterialX:

        outputFilePath = mx.FilePath()
        outputFilePath = inputFilePath
        print('Converting glTF file:%s to MaterialX file: %s' % 
              (inputFilePath.asString(), inputFilePath.asString()+".mtlx"))
        
        createAssignments = False
        fullDefinition = False
        handler.setDefinitions(stdlib)
        handler.setGenerateAssignments(createAssignments)
        handler.setGenerateFullDefinitions(fullDefinition)
    
        loadedMaterial = handler.load(inputFilePath, log)
        print("- Loaded GLTF file: " + str(loadedMaterial))
        doc = None
        if loadedMaterial:
            doc = handler.getMaterials() 
        if doc:
            # Filter out standard library elements
            writeOptions = mx.XmlWriteOptions()
            writeOptions.writeXIncludeEnable = False
            writeOptions.elementPredicate = skipLibraryElement
            mx.writeToXmlFile(doc, inputFilePath.asString()+".mtlx", writeOptions)

        print("- Converted: " + str(doc != None))

    if log:
        print("Log" + '\n'.join(log))


if __name__ == '__main__':
    main()
