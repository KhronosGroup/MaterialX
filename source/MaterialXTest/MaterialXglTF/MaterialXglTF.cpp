
#include <MaterialXTest/Catch/catch.hpp>

#include <MaterialXglTF/CgltfMaterialLoader.h>
#include <MaterialXFormat/Environ.h>
#include <MaterialXFormat/Util.h>
#include <MaterialXFormat/XmlIo.h>

#include <fstream>
#include <iostream>
#include <limits>
#include <unordered_set>

namespace mx = MaterialX;

mx::DocumentPtr glTF2Mtlx(const mx::FilePath& filename, mx::DocumentPtr definitions, 
                          bool createAssignments, bool fullDefinition)
{
    mx::MaterialLoaderPtr gltfMTLXLoader = mx::CgltfMaterialLoader::create();
    gltfMTLXLoader->setDefinitions(definitions);
    gltfMTLXLoader->setGenerateAssignments(createAssignments);
    gltfMTLXLoader->setGenerateFullDefinitions(fullDefinition);
    bool loadedMaterial = gltfMTLXLoader->load(filename);
    mx::DocumentPtr materials = loadedMaterial ? gltfMTLXLoader->getMaterials() : nullptr;
    return materials;
}

// MaterialX to cgTF conversion
bool mtlx2glTF(const mx::FilePath& filename, mx::DocumentPtr materials)
{
    mx::MaterialLoaderPtr gltfMTLXLoader = mx::CgltfMaterialLoader::create();
    gltfMTLXLoader->setMaterials(materials);
    return gltfMTLXLoader->save(filename);
}

TEST_CASE("glTF: Valid glTF -> MTLX", "[gltf]")
{
    mx::DocumentPtr libraries = mx::createDocument();
    mx::FileSearchPath searchPath;
    searchPath.append(mx::FilePath::getCurrentPath());
    mx::StringSet xincludeFiles = loadLibraries({ "libraries" }, searchPath, libraries);

    mx::XmlWriteOptions writeOptions;
    writeOptions.elementPredicate = [](mx::ConstElementPtr elem)
    {
        if (elem->hasSourceUri())
        {
            return false;
        }
        return true;
    };

    // Scan for glTF files
    bool createAssignments = true;
    bool fullDefinition = false;
    const std::string GLTF_EXTENSION("gltf");
    const mx::FilePath currentPath = mx::FilePath::getCurrentPath();
    mx::FilePath rootPath = currentPath / "resources/Materials/TestSuite/glTF/2.0/";
    for (const mx::FilePath& dir : rootPath.getSubDirectories())
    {
        if (std::string::npos != dir.asString().find("glTF-Binary") || 
            std::string::npos != dir.asString().find("glTF-Draco") || 
            std::string::npos != dir.asString().find("glTF-Embedded"))
        {
            continue;
        }
        for (const mx::FilePath& gltfFile : dir.getFilesInDirectory(GLTF_EXTENSION))
        {
            mx::FilePath fullPath = dir / gltfFile;
            mx::DocumentPtr materials = glTF2Mtlx(fullPath, libraries, createAssignments, fullDefinition);
            if (materials)
            {
                std::vector<mx::NodePtr> nodes = materials->getMaterialNodes();
                if (nodes.size())
                {
                    const std::string outputFileName = fullPath.asString() + "_converted.mtlx";
                    std::cout << "Wrote " << std::to_string(nodes.size()) << " materials to file : " << outputFileName << std::endl;
                    mx::writeToXmlFile(materials, outputFileName, &writeOptions);
                }
            }
        }
    }
}