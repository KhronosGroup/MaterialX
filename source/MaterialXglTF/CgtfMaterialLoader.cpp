
#include <MaterialXCore/Value.h>
#include <MaterialXCore/Types.h>
#include <MaterialXCore/Library.h>
#include <MaterialXFormat/XmlIo.h>
#include <MaterialXFormat/Util.h>
#include <MaterialXglTF/CgltfMaterialLoader.h>

#if defined(__GNUC__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wswitch"
    #ifndef __clang__
        #if __GNUC_PREREQ(12,0)
            #pragma GCC diagnostic ignored "-Wformat-truncation"
        #endif
    #endif
#endif

#if defined(_MSC_VER)
    #pragma warning(push)
    #pragma warning(disable : 4996)
#endif

//#define CGLTF_IMPLEMENTATION -- don't set to avoid duplicate symbols
#include <MaterialXRender/External/Cgltf/cgltf.h>
#define CGLTF_WRITE_IMPLEMENTATION
#include <MaterialXRender/External/Cgltf/cgltf_write.h>

#if defined(_MSC_VER)
    #pragma warning(pop)
#endif

#if defined(__GNUC__)
    #pragma GCC diagnostic pop
#endif

#include <cstring>
#include <iostream>
#include <limits>
#include <algorithm>
#include <sstream>
#include <iterator>

MATERIALX_NAMESPACE_BEGIN

namespace
{
const std::string SPACE_STRING = " ";
using GLTFMaterialMeshList = std::unordered_map<string, string>;
const std::string DEFAULT_NODE_PREFIX = "NODE_";
const std::string DEFAULT_MESH_PREFIX = "MESH_";
const std::string DEFAULT_MATERIAL_NAME = "MATERIAL_0";
const std::string DEFAULT_SHADER_NAME = "SHADER_0";

void initialize_cgltf_texture_view(cgltf_texture_view& textureview)
{
    std::memset(&textureview, 0, sizeof(cgltf_texture_view));
    textureview.texture = nullptr;
    textureview.scale = 1.0;
    textureview.has_transform = false;
    textureview.extras.start_offset = 0;
    textureview.extras.end_offset = 0;
    textureview.extensions_count = 0;
    textureview.extensions = nullptr;
}

void initialize_cgtlf_texture(cgltf_texture& texture, const string& name, const string& uri, 
                              cgltf_image* image)
{
    std::memset(&texture, 0, sizeof(cgltf_texture));
    texture.has_basisu = false;
    texture.extras.start_offset = 0;
    texture.extras.end_offset = 0;
    texture.extensions_count = 0;
    texture.sampler = nullptr;
    texture.image = image;
    texture.image->extras.start_offset = 0;
    texture.image->extras.end_offset = 0;
    texture.image->extensions_count = 0;
    texture.image->buffer_view = nullptr;
    texture.image->mime_type = nullptr;
    texture.image->name = const_cast<char*>((new string(name))->c_str());
    texture.name = texture.image->name;
    texture.image->uri = const_cast<char*>((new string(uri))->c_str());
}

void computeMeshMaterials(GLTFMaterialMeshList& materialMeshList, void* cnodeIn, FilePath& path, unsigned int nodeCount,
                          unsigned int meshCount)
{
    cgltf_node* cnode = static_cast<cgltf_node*>(cnodeIn);

    // Push node name on to path
    FilePath prevPath = path;
    string cnodeName = cnode->name ? string(cnode->name) : DEFAULT_NODE_PREFIX + std::to_string(nodeCount++);
    path = path / ( createValidName(cnodeName) + "/" );
    cgltf_mesh* cmesh = cnode->mesh;
    if (cmesh)
    {
        // Set path to mesh if no transform path found
        if (path.isEmpty())
        {
            string meshName = cmesh->name ? string(cmesh->name) : DEFAULT_MESH_PREFIX + std::to_string(meshCount++);
            path = createValidName(meshName);
        }

        cgltf_primitive* prim = cmesh->primitives;
        if (prim && prim->material)
        {
            cgltf_material* material = prim->material;
            if (material)
            {
                // Add reference to mesh (by name) to material 
                string stringPath = path.asString(FilePath::FormatPosix);
                string materialName = material->name;
                if (materialMeshList.count(materialName))
                {
                    materialMeshList[materialName].append(", " + stringPath);
                }
                else
                {
                    materialMeshList.insert({ materialName, stringPath });
                }
            }
        }
    }

    for (cgltf_size i = 0; i < cnode->children_count; i++)
    {
        if (cnode->children[i])
        {
            computeMeshMaterials(materialMeshList, cnode->children[i], path, nodeCount, meshCount);
        }
    }

    // Pop path name
    path = prevPath;
}

}

bool CgltfMaterialLoader::save(const FilePath& filePath)
{
    if (!_materials)
    {
        return false;
    }

    const string input_filename = filePath.asString();
    const string ext = stringToLower(filePath.getExtension());
    const string BINARY_EXTENSION = "glb";
    const string ASCII_EXTENSION = "gltf";
    if (ext != BINARY_EXTENSION && ext != ASCII_EXTENSION)
    {
        return false;
    }

    cgltf_options options;
    std::memset(&options, 0, sizeof(options));
    cgltf_data* data = new cgltf_data();
	data->file_type = (ext == BINARY_EXTENSION) ? cgltf_file_type::cgltf_file_type_glb : cgltf_file_type::cgltf_file_type_gltf;
    data->file_data = nullptr;
	//cgltf_asset asset;
	data->meshes = nullptr;
	data->meshes_count = 0;
	data->materials = nullptr;
	data->materials_count = 0;
	data->accessors = nullptr;
	data->accessors_count = 0;
	data->buffer_views = nullptr;
	data->buffer_views_count = 0;
	data->buffers = nullptr;
	data->buffers_count = 0;
	data->images = nullptr;
	data->images_count = 0;
	data->textures = nullptr;
	data->textures_count = 0;
	data->samplers = nullptr;
	data->samplers_count = 0;
	data->skins = nullptr;
    data->skins_count = 0;
    data->cameras = nullptr;
	data->cameras_count = 0;
	data->lights = nullptr;
	data->lights_count = 0;
	data->nodes = nullptr;
	data->nodes_count = 0;
	data->scenes = nullptr;
    data->scenes_count = 0;
	data->scene = nullptr;
	data->animations = nullptr;
	data->animations_count = 0;
	data->variants = nullptr;
	data->variants_count = 0;
	//cgltf_extras extras;
	data->data_extensions_count = 0;
	data->data_extensions = nullptr;
	data->extensions_used = nullptr;
	data->extensions_used_count = 0;
	data->extensions_required = nullptr;
	data->extensions_required_count = 0;
	data->json = nullptr;
	data->json_size = 0;
	data->bin = nullptr;
	data->bin_size = 0;

	data->asset.generator = const_cast<char*>((new string("MaterialX 1.38.4 to glTF generator"))->c_str());;
    data->asset.version = const_cast<char*>((new string("1.38"))->c_str());
	data->asset.min_version = const_cast<char*>((new string("1.38"))->c_str());;

    // Scan for PBR shader nodes
    const string PBR_CATEGORY_STRING("gltf_pbr");
    std::set<NodePtr> pbrNodes;
    for (const NodePtr& material : _materials->getMaterialNodes())
    {
        vector<NodePtr> shaderNodes = getShaderNodes(material);
        for (const NodePtr& shaderNode : shaderNodes)
        {
            if (shaderNode->getCategory() == PBR_CATEGORY_STRING &&
                pbrNodes.find(shaderNode) == pbrNodes.end())
            {
                pbrNodes.insert(shaderNode);
            }
        }
    }

    cgltf_size materials_count = pbrNodes.size();
    if (!materials_count)
    {
        return false;
    }

    // Write materials
    /*
    * typedef struct cgltf_material
    {
	    char* name;
	    cgltf_bool has_pbr_metallic_roughness;
	    cgltf_bool has_pbr_specular_glossiness;
	    cgltf_bool has_clearcoat;
	    cgltf_bool has_transmission;
	    cgltf_bool has_volume;
	    cgltf_bool has_ior;
	    cgltf_bool has_specular;
	    cgltf_bool has_sheen;
	    cgltf_bool has_emissive_strength;
	    cgltf_pbr_metallic_roughness pbr_metallic_roughness;
	    cgltf_pbr_specular_glossiness pbr_specular_glossiness;
	    cgltf_clearcoat clearcoat;
	    cgltf_ior ior;
	    cgltf_specular specular;
	    cgltf_sheen sheen;
	    cgltf_transmission transmission;
	    cgltf_volume volume;
	    cgltf_emissive_strength emissive_strength;
	    cgltf_texture_view normal_texture;
	    cgltf_texture_view occlusion_texture;
	    cgltf_texture_view emissive_texture;
	    cgltf_float emissive_factor[3];
	    cgltf_alpha_mode alpha_mode;
	    cgltf_float alpha_cutoff;
	    cgltf_bool double_sided;
	    cgltf_bool unlit;
	    cgltf_extras extras;
	    cgltf_size extensions_count;
	    cgltf_extension* extensions;
    } cgltf_material;
    */
    cgltf_material* materials = new cgltf_material[materials_count];
    data->materials = materials;
    data->materials_count = materials_count;

    // Set of image nodes.
    // TODO: Fix to be dynamic
    cgltf_texture textureList[1024];
    cgltf_image imageList[1024];

    size_t material_idx = 0;
    size_t imageIndex = 0;
    for (const NodePtr& pbrNode : pbrNodes)
    {
        cgltf_material* material = &(materials[material_idx]);
        std::memset(material, 0, sizeof(cgltf_material));
	    material->has_pbr_metallic_roughness = false;
	    material->has_pbr_specular_glossiness = false;
	    material->has_clearcoat = false;
	    material->has_transmission = false;
	    material->has_volume = false;
	    material->has_ior = false;
	    material->has_specular = false;
	    material->has_sheen = false;
	    material->has_emissive_strength = false;
	    material->extensions_count = 0;
	    material->extensions = nullptr;
        material->emissive_texture.texture = nullptr;
        material->normal_texture.texture = nullptr;
        material->occlusion_texture.texture = nullptr;

        string* name = new string(pbrNode->getNamePath());
        material->name = const_cast<char*>(name->c_str());
        
        material->has_pbr_metallic_roughness = true;
        cgltf_pbr_metallic_roughness& roughness = material->pbr_metallic_roughness;
        initialize_cgltf_texture_view(roughness.base_color_texture);

        // Handle base color
        string filename;
        NodePtr imageNode = pbrNode->getConnectedNode("base_color");
        if (imageNode)
        {
            InputPtr fileInput = imageNode->getInput("file");
            filename = fileInput && fileInput->getAttribute("type") == "filename" ?
                fileInput->getValueString() : EMPTY_STRING;
            if (filename.empty())
                imageNode = nullptr;
        }
        if (imageNode)
        {
            cgltf_texture* texture = &(textureList[imageIndex]);
            roughness.base_color_texture.texture = texture;
            initialize_cgtlf_texture(*texture, imageNode->getNamePath(), filename,
                &(imageList[imageIndex]));            

            roughness.base_color_factor[0] = 1.0;
            roughness.base_color_factor[1] = 1.0;
            roughness.base_color_factor[2] = 1.0;
            roughness.base_color_factor[3] = 1.0;

            imageIndex++;

            // Pull off color from gltf_coloredImage node
            ValuePtr value = pbrNode->getInputValue("color");
            if (value && value->isA<Color4>())
            {
                Color4 color = value->asA<Color4>();
                roughness.base_color_factor[0] = color[0];
                roughness.base_color_factor[1] = color[1];
                roughness.base_color_factor[2] = color[2];
                roughness.base_color_factor[3] = color[3];
            }
        }
        else
        {
            ValuePtr value = pbrNode->getInputValue("base_color");
            if (value)
            {
                Color3 color = value->asA<Color3>();
                roughness.base_color_factor[0] = color[0];
                roughness.base_color_factor[1] = color[1];
                roughness.base_color_factor[2] = color[2];
            }

            value = pbrNode->getInputValue("alpha");
            if (value)
            {
                roughness.base_color_factor[3] = value->asA<float>();
            }
        }

        // Handle metallic, roughness, occlusion
        initialize_cgltf_texture_view(roughness.metallic_roughness_texture);
        ValuePtr value;
        string extractInputs[3] =
        {
            "metallic",
            "roughness",
            "occlusion"
        };
        cgltf_float* roughnessInputs[3] =
        {
            &roughness.metallic_factor,
            &roughness.roughness_factor,
            nullptr
        };

        NodePtr ormNode = nullptr;
        imageNode = nullptr;
        const string extractCategory("extract");
        string mrFileName;
        for (size_t e=0; e<3; e++)
        { 
            const string& inputName = extractInputs[e];
            InputPtr pbrInput = pbrNode->getInput(inputName);            
            if (pbrInput)
            {
                // Read past any extract node
                NodePtr connectedNode = pbrNode->getConnectedNode(inputName);
                if (connectedNode)
                {
                    if (connectedNode->getCategory() == extractCategory)
                    {
                        imageNode = connectedNode->getConnectedNode("in");
                    }
                    else
                    {
                        imageNode = connectedNode;
                    }
                }

                if (imageNode)
                {
                    InputPtr fileInput = imageNode->getInput("file");
                    filename = fileInput && fileInput->getAttribute("type") == "filename" ?
                        fileInput->getValueString() : EMPTY_STRING;
                    if (inputName != "occlusion")
                    {
                        mrFileName = filename;

                        // Only create the ORM texture once
                        if (!ormNode)
                        {
                            ormNode = imageNode;
                            cgltf_texture* texture = &(textureList[imageIndex]);
                            roughness.metallic_roughness_texture.texture = texture;
                            initialize_cgtlf_texture(*texture, imageNode->getNamePath(), filename,
                                &(imageList[imageIndex]));
                            imageIndex++;
                        }
                    }
                    else
                    {
                        // If the occlusion file is not the same as the metallic roughness,
                        // then create another texture.
                        if (mrFileName != filename)
                        {
                            cgltf_texture* texture = &(textureList[imageIndex]);
                            material->occlusion_texture.texture = texture;
                            initialize_cgtlf_texture(*texture, imageNode->getNamePath(), filename,
                                &(imageList[imageIndex]));
                            imageIndex++;
                        }
                    }

                    if (roughnessInputs[e])
                    {
                        *(roughnessInputs[e]) = 1.0f;
                    }
                }
                else
                {
                    if (roughnessInputs[e])
                    {
                        value = pbrInput->getValue();
                        *(roughnessInputs[e]) = value->asA<float>();
                    }
                }
            }
        }

        // Handle normal
        filename = EMPTY_STRING;
        imageNode = pbrNode->getConnectedNode("normal");
        initialize_cgltf_texture_view(material->normal_texture);
        if (imageNode)
        {
            // Read past normalmap node
            if (imageNode->getCategory() == "normalmap")
            {
                imageNode = imageNode->getConnectedNode("in");
            }
            if (imageNode)
            {
                InputPtr fileInput = imageNode->getInput("file");
                filename = fileInput && fileInput->getAttribute("type") == "filename" ?
                    fileInput->getValueString() : EMPTY_STRING;
                if (filename.empty())
                    imageNode = nullptr;
            }
        }
        if (imageNode)
        {
            cgltf_texture* texture = &(textureList[imageIndex]);
            material->normal_texture.texture = texture;
            initialize_cgtlf_texture(*texture, imageNode->getNamePath(), filename,
                &(imageList[imageIndex]));            

            imageIndex++;
        }

        // Handle sheen
        cgltf_sheen& sheen = material->sheen;
        filename = EMPTY_STRING;
        imageNode = pbrNode->getConnectedNode("sheen_color");
        initialize_cgltf_texture_view(sheen.sheen_color_texture);
        if (imageNode)
        {
            InputPtr fileInput = imageNode->getInput("file");
            filename = fileInput && fileInput->getAttribute("type") == "filename" ?
                fileInput->getValueString() : EMPTY_STRING;
            if (filename.empty())
                imageNode = nullptr;
        }
        if (imageNode)
        {
            cgltf_texture* texture = &(textureList[imageIndex]);
            sheen.sheen_color_texture.texture = texture;
            initialize_cgtlf_texture(*texture, imageNode->getNamePath(), filename,
                &(imageList[imageIndex]));            

            sheen.sheen_color_factor[0] = 1.0;
            sheen.sheen_color_factor[1] = 1.0;
            sheen.sheen_color_factor[2] = 1.0;

            imageIndex++;
        }
        else
        {
            value = pbrNode->getInputValue("sheen_color");
            if (value)
            {
                Color3 color = value->asA<Color3>();
                sheen.sheen_color_factor[0] = color[0];
                sheen.sheen_color_factor[1] = color[1];
                sheen.sheen_color_factor[2] = color[2];
            }
        }        

        filename = EMPTY_STRING;
        imageNode = pbrNode->getConnectedNode("sheen_roughness");
        initialize_cgltf_texture_view(sheen.sheen_roughness_texture);
        if (imageNode)
        {
            InputPtr fileInput = imageNode->getInput("file");
            filename = fileInput && fileInput->getAttribute("type") == "filename" ?
                fileInput->getValueString() : EMPTY_STRING;
            if (filename.empty())
                imageNode = nullptr;
        }
        if (imageNode)
        {
            cgltf_texture* texture = &(textureList[imageIndex]);
            sheen.sheen_roughness_texture.texture = texture;
            initialize_cgtlf_texture(*texture, imageNode->getNamePath(), filename,
                &(imageList[imageIndex]));            

            sheen.sheen_roughness_factor = 1.0;

            imageIndex++;
        }
        else
        {
            value = pbrNode->getInputValue("sheen_roughness");
            if (value)
            {
                sheen.sheen_roughness_factor = value->asA<float>();
            }
        }

        // Handle emissive
        filename = EMPTY_STRING;
        imageNode = pbrNode->getConnectedNode("emissive");
        initialize_cgltf_texture_view(material->emissive_texture);
        if (imageNode)
        {
            InputPtr fileInput = imageNode->getInput("file");
            filename = fileInput && fileInput->getAttribute("type") == "filename" ?
                fileInput->getValueString() : EMPTY_STRING;
            if (filename.empty())
                imageNode = nullptr;
        }
        if (imageNode)
        {
            cgltf_texture* texture = &(textureList[imageIndex]);
            material->emissive_texture.texture = texture;
            initialize_cgtlf_texture(*texture, imageNode->getNamePath(), filename,
                &(imageList[imageIndex]));            

            material->emissive_factor[0] = 1.0;
            material->emissive_factor[1] = 1.0;
            material->emissive_factor[2] = 1.0;

            imageIndex++;
        }
        else
        {
            value = pbrNode->getInputValue("emissive");
            if (value)
            {
                Color3 color = value->asA<Color3>();
                material->emissive_factor[0] = color[0];
                material->emissive_factor[1] = color[1];
                material->emissive_factor[2] = color[2];
            }
        }        
        
        value = pbrNode->getInputValue("emissive_strength");
        if (value)
        {
            material->emissive_strength.emissive_strength = value->asA<float>();
        }

        material_idx++;
    }

    // Set image and texture lists
    data->images_count = imageIndex;
    data->images = &imageList[0];
    data->textures_count = imageIndex; 
    data->textures = &textureList[0];

    // Write to disk
    cgltf_result result = cgltf_write_file(&options, filePath.asString().c_str(), data);
    if (result != cgltf_result_success)
    {
        return false;
    }
    return true;
}

bool CgltfMaterialLoader::load(const FilePath& filePath)
{
    const std::string input_filename = filePath.asString();
    const std::string ext = stringToLower(filePath.getExtension());
    const std::string BINARY_EXTENSION = "glb";
    const std::string ASCII_EXTENSION = "gltf";
    if (ext != BINARY_EXTENSION && ext != ASCII_EXTENSION)
    {
        return false;
    }

    cgltf_options options;
    std::memset(&options, 0, sizeof(options));
    cgltf_data* data = nullptr;

    // Read file
    cgltf_result result = cgltf_parse_file(&options, input_filename.c_str(), &data);
    if (result != cgltf_result_success)
    {
        return false;
    }
    if (cgltf_load_buffers(&options, data, input_filename.c_str()) != cgltf_result_success)
    {
        return false;
    }

    loadMaterials(data);

    if (data)
    {
        cgltf_free(data);
    }

    return true;
}

// Utilities
NodePtr CgltfMaterialLoader::createColoredTexture(DocumentPtr& doc, const std::string & nodeName, const std::string& fileName,
                                                  const Color4& color, const std::string & colorspace)
{
    std::string newTextureName = doc->createValidChildName(nodeName);
    NodePtr newTexture = doc->addNode("gltf_coloredimage", newTextureName, "multioutput");
    newTexture->setAttribute("nodedef", "ND_gltf_colortiledimage" );
    if (_generateFullDefinitions)
    {
        newTexture->addInputsFromNodeDef();
    }
    InputPtr fileInput = newTexture->addInputFromNodeDef("file");
    fileInput->setValue(fileName, "filename");

    InputPtr colorInput = newTexture->addInputFromNodeDef("color");
    ValuePtr colorValue = Value::createValue<Color4>(color);
    const string& cvs = colorValue->getValueString();
    colorInput->setValueString(cvs);

    if (!colorspace.empty())
    {
        fileInput->setAttribute("colorspace", colorspace);
    }
    return newTexture;
}


NodePtr CgltfMaterialLoader::createTexture(DocumentPtr& doc, const std::string & nodeName, const std::string& fileName,
                                           const std::string & textureType, const std::string & colorspace)
{
    std::string newTextureName = doc->createValidChildName(nodeName);
    NodePtr newTexture = doc->addNode("tiledimage", newTextureName, textureType);
    newTexture->setAttribute("nodedef", "ND_image_" + textureType);
    if (_generateFullDefinitions)
    {
        newTexture->addInputsFromNodeDef();
    }
    InputPtr fileInput = newTexture->addInputFromNodeDef("file");
    fileInput->setValue(fileName, "filename");
    if (!colorspace.empty())
    {
        fileInput->setAttribute("colorspace", colorspace);
    }
    return newTexture;
}


void CgltfMaterialLoader::setNormalMapInput(DocumentPtr materials, NodePtr shaderNode, const std::string& inputName,
    const void* textureViewIn, const std::string& inputImageNodeName)
{
    const cgltf_texture_view* textureView = static_cast<const cgltf_texture_view*>(textureViewIn);

    InputPtr normalInput = shaderNode->addInputFromNodeDef(inputName);
    if (normalInput)
    {
        cgltf_texture* texture = textureView ? textureView->texture : nullptr;
        if (texture && texture->image)
        {
            std::string imageNodeName = texture->image->name ? texture->image->name :
                inputImageNodeName;
            imageNodeName = _materials->createValidChildName(imageNodeName);
            std::string uri = texture->image->uri ? texture->image->uri : SPACE_STRING;
            NodePtr newTexture = createTexture(_materials, imageNodeName, uri,
                "vector3", EMPTY_STRING);

            std::string normalMapName = _materials->createValidChildName("pbr_normalmap");
            NodePtr normalMap = _materials->addNode("normalmap", normalMapName, "vector3");
            normalMap->setAttribute("nodedef", "ND_normalmap");
            if (_generateFullDefinitions)
            {
                normalMap->addInputsFromNodeDef();
            }
            else
            {
                normalMap->addInputFromNodeDef("in");
            }
            normalMap->getInput("in")->setAttribute("nodename", newTexture->getName());
            normalMap->getInput("in")->setType("vector3");

            normalInput->setAttribute("nodename", normalMap->getName());
        }
    }
}

void CgltfMaterialLoader::setColorInput(DocumentPtr materials, NodePtr shaderNode, const std::string& colorInputName, 
                                       const Color3& color, float alpha, const std::string& alphaInputName, 
                                       const void* textureViewIn,
                                       const std::string& inputImageNodeName)
{
    const cgltf_texture_view* textureView = static_cast<const cgltf_texture_view*>(textureViewIn);

    InputPtr colorInput = colorInputName.size() ? shaderNode->addInputFromNodeDef(colorInputName) : nullptr;    
    InputPtr alphaInput = alphaInputName.size() ? shaderNode->addInputFromNodeDef(alphaInputName) : nullptr;
    if (!colorInput)
    {
        return;
    } 

    // Handle textured color / alpha input
    //
    cgltf_texture* texture = textureView ? textureView->texture : nullptr;
    if (texture && texture->image)
    {
        // Simple color3 image lookup
        if (!alphaInput)
        {
            std::string imageNodeName = texture->image->name ? texture->image->name : inputImageNodeName;
            imageNodeName = materials->createValidChildName(imageNodeName);
            std::string uri = texture->image->uri ? texture->image->uri : SPACE_STRING;
            NodePtr newTexture = createTexture(materials, imageNodeName, uri, "color3", "srgb_texture");
            colorInput->setAttribute("nodename", newTexture->getName());
            colorInput->removeAttribute("value");
        }

        // Color, alpha lookup
        else
        {
            std::string imageNodeName = texture->image->name ? texture->image->name : inputImageNodeName;
            imageNodeName = materials->createValidChildName(imageNodeName);
            std::string uri = texture->image->uri ? texture->image->uri : SPACE_STRING;

            const Color4 color4(color[0], color[1], color[2], alpha);
            NodePtr newTexture = createColoredTexture(materials, imageNodeName, uri, color4, "srgb_texture");

            const string& newTextureName = newTexture->getName();
            colorInput->setAttribute("nodename", newTextureName);
            colorInput->setOutputString("outcolor");
            colorInput->removeAttribute("value");

            alphaInput->setAttribute("nodename", newTextureName);
            alphaInput->setOutputString("outa");
            alphaInput->removeAttribute("value");
        }
    }

    // Handle simple color / alpha input
    else
    {
        if (colorInput)
        {
            ValuePtr color3Value = Value::createValue<Color3>(color);
            colorInput->setValueString(color3Value->getValueString());
        }
        if (alphaInput)
        {
            alphaInput->setValue<float>(alpha);
        }
    }
}

void CgltfMaterialLoader::setFloatInput(DocumentPtr materials, NodePtr shaderNode, const std::string& inputName, 
                                       float floatFactor, const void* textureViewIn,
                                       const std::string& inputImageNodeName)
{
    const cgltf_texture_view* textureView = static_cast<const cgltf_texture_view*>(textureViewIn);

    InputPtr floatInput = shaderNode->addInputFromNodeDef(inputName);
    if (floatInput)
    {
        cgltf_texture* texture = textureView ? textureView->texture : nullptr;
        if (texture && texture->image)
        {
            std::string imageNodeName = materials->createValidChildName(inputImageNodeName);
            std::string uri = texture->image->uri ? texture->image->uri : SPACE_STRING;
            NodePtr newTexture = createTexture(materials, imageNodeName, uri,
                "float", EMPTY_STRING);
            floatInput->setAttribute("nodename", newTexture->getName());
            floatInput->setValue<float>(floatFactor);
        }
        else
        {
            floatInput->setValue<float>(floatFactor);
        }
    }
}

void CgltfMaterialLoader::loadMaterials(void *vdata)
{
    cgltf_data* data = static_cast<cgltf_data*>(vdata);

    // Scan materials
    /*
    * typedef struct cgltf_material
    {
	    char* name;
	    cgltf_bool has_pbr_metallic_roughness;
	    cgltf_bool has_pbr_specular_glossiness;
	    cgltf_bool has_clearcoat;
	    cgltf_bool has_transmission;
	    cgltf_bool has_volume;
	    cgltf_bool has_ior;
	    cgltf_bool has_specular;
	    cgltf_bool has_sheen;
	    cgltf_bool has_emissive_strength;
	    cgltf_pbr_metallic_roughness pbr_metallic_roughness;
	    cgltf_pbr_specular_glossiness pbr_specular_glossiness;
	    cgltf_clearcoat clearcoat;
	    cgltf_ior ior;
	    cgltf_specular specular;
	    cgltf_sheen sheen;
	    cgltf_transmission transmission;
	    cgltf_volume volume;
	    cgltf_emissive_strength emissive_strength;
	    cgltf_texture_view normal_texture;
	    cgltf_texture_view occlusion_texture;
	    cgltf_texture_view emissive_texture;
	    cgltf_float emissive_factor[3];
	    cgltf_alpha_mode alpha_mode;
	    cgltf_float alpha_cutoff;
	    cgltf_bool double_sided;
	    cgltf_bool unlit;
	    cgltf_extras extras;
	    cgltf_size extensions_count;
	    cgltf_extension* extensions;
    } cgltf_material;
    */
    if (!data->materials_count)
    {
        _materials = nullptr;
        return;
    }
    _materials = Document::createDocument<Document>();
    _materials->importLibrary(_definitions);

    for (size_t m = 0; m < data->materials_count; m++)
    {
        cgltf_material* material = &(data->materials[m]);
        if (!material)
        {
            continue;
        }

        // Create a default gltf_pbr node
        std::string shaderName;
        std::string materialName;
        if (!material->name)
        {
            materialName = DEFAULT_MATERIAL_NAME;
            shaderName = DEFAULT_SHADER_NAME;
        }
        else
        {
            StringMap invalidTokens;
            invalidTokens["__"] = "_";
            string origName = material->name;
            origName = replaceSubstrings(origName, invalidTokens);
            
            materialName = "MAT_" + origName;
            shaderName = "SHD_" + origName;
        }
        materialName = _materials->createValidChildName(materialName);
        string* name = new string(materialName);
        material->name = const_cast<char*>(name->c_str());

        shaderName = _materials->createValidChildName(shaderName);

        // Check for unlit
        bool use_unlit = material->unlit;
        string shaderCategory = use_unlit ? "surface_unlit" : "gltf_pbr";
        NodePtr shaderNode = _materials->addNode(shaderCategory, shaderName, "surfaceshader");
        //shaderNode->setAttribute("nodedef", "ND_gltf_pbr_surfaceshader");
        if (_generateFullDefinitions)
        {
            shaderNode->addInputsFromNodeDef();
        }

        // Create a surface material for the shader node
        NodePtr materialNode = _materials->addNode("surfacematerial", materialName, "material");
        InputPtr shaderInput = materialNode->addInput("surfaceshader", "surfaceshader");
        shaderInput->setAttribute("nodename", shaderNode->getName());        

        // Handle separate occlusion texture
        bool haveSeparateOcclusion = false;
        cgltf_texture* occlusion_texture = material->occlusion_texture.texture;
        if (!use_unlit && occlusion_texture)
        {
            std::string oURI;
            std::string mrURI;
            if (occlusion_texture->image)
            {
                oURI = occlusion_texture->image->uri ? occlusion_texture->image->uri : SPACE_STRING;
            }
            if (!oURI.empty() && material->has_pbr_metallic_roughness)
            {
                cgltf_pbr_metallic_roughness& roughness = material->pbr_metallic_roughness;
                cgltf_texture_view& textureView = roughness.metallic_roughness_texture;
                cgltf_texture* texture = textureView.texture;
                if (texture && texture->image)
                {
                    mrURI = texture->image->uri ? texture->image->uri : SPACE_STRING;
                }

                if (mrURI != oURI)
                {
                    haveSeparateOcclusion = true;
                    InputPtr occlusionInput = shaderNode->addInputFromNodeDef("occlusion");
                    setFloatInput(_materials, shaderNode, "occlusion", 1.0, &material->occlusion_texture, "image_occlusion");
                }
            }
        }

        if (material->has_pbr_metallic_roughness)
        {
            StringVec colorAlphaInputs = { "base_color", "alpha", "emission_color", "opacity" };
            size_t  colorAlphaInputOffset = use_unlit ? 2 : 0;

            cgltf_pbr_metallic_roughness& roughness = material->pbr_metallic_roughness;

            // Parse base color and alpha
            Color3 colorFactor(roughness.base_color_factor[0],
                roughness.base_color_factor[1],
                roughness.base_color_factor[2]);
            float alpha = roughness.base_color_factor[3];
            setColorInput(_materials, shaderNode, colorAlphaInputs[colorAlphaInputOffset],
                colorFactor, alpha, colorAlphaInputs[colorAlphaInputOffset+1], 
                &roughness.base_color_texture, "image_basecolor");

            // Ignore any other information unsupported by unlit.
            if (use_unlit)
            {
                continue;
            }

            // Parse metalic, roughness, and occlusion (if not specified separately)
            InputPtr metallicInput = shaderNode->addInputFromNodeDef("metallic");
            InputPtr roughnessInput = shaderNode->addInputFromNodeDef("roughness");
            InputPtr occlusionInput = haveSeparateOcclusion ? nullptr : shaderNode->addInputFromNodeDef("occlusion");

            // Check for occlusion/metallic/roughness texture
            cgltf_texture_view& textureView = roughness.metallic_roughness_texture;
            cgltf_texture* texture = textureView.texture;
            if (texture && texture->image)
            {
                std::string imageNodeName = texture->image->name ? texture->image->name :
                    "image_orm";
                imageNodeName = _materials->createValidChildName(imageNodeName);
                std::string uri = texture->image->uri ? texture->image->uri : SPACE_STRING;
                NodePtr textureNode = createTexture(_materials, imageNodeName, uri,
                    "vector3", EMPTY_STRING);

                // Map image channesl to inputs
                StringVec indexName = { "x", "y", "z" };
                std::vector<InputPtr> inputs = { occlusionInput, roughnessInput, metallicInput };
                for (size_t i = 0; i < inputs.size(); i++)
                {
                    if (inputs[i])
                    {
                        inputs[i]->setAttribute("nodename", textureNode->getName());
                        inputs[i]->setType("float");
                        inputs[i]->setChannels(indexName[i]);
                    }
                }

                // See note about both "value" and "nodename" being
                // specified.
                metallicInput->setValue<float>(roughness.metallic_factor);
                roughnessInput->setValue<float>(roughness.roughness_factor);
            }
            else
            {
                metallicInput->setValue<float>(roughness.metallic_factor);;
                roughnessInput->setValue<float>(roughness.roughness_factor);
            }
        }

        // THIS IS NOT SUPPORTED. Users are expected to perform pre-conversion. 
        // For now hust some basic testing code here.
        // // typedef struct cgltf_pbr_specular_glossiness
        // {
        //	cgltf_texture_view diffuse_texture;
        //	cgltf_texture_view specular_glossiness_texture;
        //
        //	cgltf_float diffuse_factor[4];
        //	cgltf_float specular_factor[3];
        //	cgltf_float glossiness_factor;
        // } cgltf_pbr_specular_glossiness;
        //
        else if (material->has_pbr_specular_glossiness && !use_unlit)
        {
            StringVec colorAlphaInputs = { "base_color", "alpha" };
            size_t  colorAlphaInputOffset = 0;

            cgltf_pbr_specular_glossiness& specgloss = material->pbr_specular_glossiness;

            // Parse base color and alpha
            Color3 colorFactor(specgloss.diffuse_factor[0],
                specgloss.diffuse_factor[1],
                specgloss.diffuse_factor[2]);
            float alpha = specgloss.diffuse_factor[3];
            setColorInput(_materials, shaderNode, colorAlphaInputs[colorAlphaInputOffset],
                colorFactor, alpha, colorAlphaInputs[colorAlphaInputOffset+1], 
                &specgloss.diffuse_texture, "image_diffusecolor");

            // Map image channesl to inputs
            InputPtr specularColorInput = shaderNode->addInputFromNodeDef("specular_color");
            InputPtr roughnessInput = shaderNode->addInputFromNodeDef("roughness");

            // Check for specular/glossiness texture
            cgltf_texture_view& textureView = specgloss.specular_glossiness_texture;
            cgltf_texture* texture = textureView.texture;
            if (texture && texture->image)
            {
                std::string imageNodeName = texture->image->name ? texture->image->name :
                    "image_specGlossiness";
                imageNodeName = _materials->createValidChildName(imageNodeName);
                std::string uri = texture->image->uri ? texture->image->uri : SPACE_STRING;
                NodePtr textureNode = createTexture(_materials, imageNodeName, uri,
                    "color4", "srgb_texture");

                StringVec channels = { "rgb", "a" };
                StringVec types = { "color3", "float" };
                std::vector<InputPtr> inputs = { specularColorInput, roughnessInput };
                for (size_t i = 0; i < inputs.size(); i++)
                {
                    if (inputs[i])
                    {
                        inputs[i]->setAttribute("nodename", textureNode->getName());
                        inputs[i]->setType(types[i]);
                        inputs[i]->setChannels(channels[i]);
                    }
                }
            }
            else
            {
                roughnessInput->setValue<float>(specgloss.glossiness_factor);
            }

            InputPtr specularInput = shaderNode->addInputFromNodeDef("specular");
            float specular = (specgloss.specular_factor[0] + specgloss.specular_factor[1] + specgloss.specular_factor[2]) / 3.0f;
            specularInput->setValue<float>(specular);;
        }

        // Firewall : Skip any other mappings as they will not exist on unlit_surface
        if (use_unlit)
        {
            continue;
        }

        // Parse unmapped alpha parameters
        //
        InputPtr alphaMode = shaderNode->addInputFromNodeDef("alpha_mode");
        if (alphaMode)
        {
            cgltf_alpha_mode alpha_mode = material->alpha_mode;
            alphaMode->setValue<int>(static_cast<int>(alpha_mode));
        }
        InputPtr alphaCutoff = shaderNode->addInputFromNodeDef("alpha_cutoff");
        if (alphaCutoff)
        {
            alphaCutoff->setValue<float>(material->alpha_cutoff);
        }

        // Normal texture
        setNormalMapInput(_materials, shaderNode, "normal", &material->normal_texture, "image_normal");

        // Handle sheen
        if (material->has_sheen)
        {
            cgltf_sheen& sheen = material->sheen;
            
            Color3 colorFactor(sheen.sheen_color_factor[0],
                               sheen.sheen_color_factor[1],
                               sheen.sheen_color_factor[2]);
            setColorInput(_materials, shaderNode, "sheen_color",
                colorFactor, 1.0f, EMPTY_STRING, &sheen.sheen_color_texture, "image_sheen");

            setFloatInput(_materials, shaderNode, "sheen_roughness",
                sheen.sheen_roughness_factor, &sheen.sheen_roughness_texture,
                "image_sheen_roughness");
        }

        // Parse clearcoat
        // typedef struct cgltf_clearcoat
        // {
        //	    cgltf_texture_view clearcoat_texture;
        //	    cgltf_texture_view clearcoat_roughness_texture;
        //	    cgltf_texture_view clearcoat_normal_texture;
        //
        //	    cgltf_float clearcoat_factor;
        //	    cgltf_float clearcoat_roughness_factor;
        // } cgltf_clearcoat;
        if (material->has_clearcoat)
        {
            cgltf_clearcoat& clearcoat = material->clearcoat;

            // Mapped or unmapped clearcoat
            setFloatInput(_materials, shaderNode, "clearcoat",
                clearcoat.clearcoat_factor, 
                &clearcoat.clearcoat_texture,
                "image_clearcoat");

            // Mapped or unmapped clearcoat roughness
            setFloatInput(_materials, shaderNode, "clearcoat_roughness",
                clearcoat.clearcoat_roughness_factor, 
                &clearcoat.clearcoat_roughness_texture,
                "image_clearcoat_roughness");           

            // Normal map clearcoat_normal
            setNormalMapInput(_materials, shaderNode, "clearcoat_normal", &material->normal_texture, 
                            "image_clearcoat_normal");

        }

        // Parse transmission
        // typedef struct cgltf_transmission
        // {
        //	    cgltf_texture_view transmission_texture;
        //	    cgltf_float transmission_factor;
        // } cgltf_transmission;
        if (material->has_transmission)
        {
            cgltf_transmission& transmission = material->transmission;

            setFloatInput(_materials, shaderNode, "transmission",
                transmission.transmission_factor, 
                &transmission.transmission_texture,
                "image_transmission");
        }

        // Parse specular and specular color
        // typedef struct cgltf_specular {
        //      cgltf_texture_view specular_texture;
        //      cgltf_texture_view specular_color_texture;
	    //      cgltf_float specular_color_factor[3];
	    //      cgltf_float specular_factor;
        // } cgltf_specular
        //
        if (material->has_specular)
        {
            cgltf_specular& specular = material->specular;

            // Mapped or unmapped specular
            Color3 colorFactor(specular.specular_color_factor[0],
                specular.specular_color_factor[1],
                specular.specular_color_factor[2]);
            setColorInput(_materials, shaderNode, "specular_color",
                colorFactor, 1.0f, EMPTY_STRING,
                &specular.specular_color_texture, 
                "image_specularcolor");

            // Mapped or unmapped specular color
            setFloatInput(_materials, shaderNode, "specular",
                specular.specular_factor, 
                &specular.specular_texture,
                "image_specular");
        }

        // Parse untextured ior 
        if (material->has_ior)
        {
            cgltf_ior& ior = material->ior;
            InputPtr iorInput = shaderNode->addInputFromNodeDef("ior");
            if (iorInput)
            {
                iorInput->setValue<float>(ior.ior);
            }
        }

        // Parse emissive inputs
        //
        // cgltf_texture_view emissive_texture;
	    // cgltf_float emissive_factor[3];
        //
        Color3 colorFactor(material->emissive_factor[0],
            material->emissive_factor[1],
            material->emissive_factor[2]);
        setColorInput(_materials, shaderNode, "emissive",
            colorFactor, 1.0f, EMPTY_STRING, &material->emissive_texture, "image_emission");

        if (material->has_emissive_strength)
        {
            cgltf_emissive_strength& emissive_strength = material->emissive_strength;
            InputPtr input = shaderNode->addInputFromNodeDef("emissive_strength");
            if (input)
            {
                input->setValue<float>(emissive_strength.emissive_strength);
            }
        }

        // Parse Volume Inputs:
        // 
        // typedef struct cgltf_volume
        // {
	    //      cgltf_texture_view thickness_texture;
	    //      cgltf_float thickness_factor;
	    //      cgltf_float attenuation_color[3];
	    //      cgltf_float attenuation_distance;
        // } cgltf_volume;
        //
        if (material->has_volume)
        {
            cgltf_volume& volume = material->volume;

            // Textured or untexture thickness
            setFloatInput(_materials, shaderNode, "thickness",
                volume.thickness_factor,
                &volume.thickness_texture,
                "thickness");

            // Untextured attenuation color
            Color3 attenFactor(volume.attenuation_color[0],
                volume.attenuation_color[1],
                volume.attenuation_color[2]);
            setColorInput(_materials, shaderNode, "attenuation_color",
                          attenFactor, 1.0f, EMPTY_STRING, nullptr, EMPTY_STRING);

            // Untextured attenuation distance
            setFloatInput(_materials, shaderNode, "attenuation_distance",
                volume.attenuation_distance, nullptr, EMPTY_STRING);
        }
    }

    // Create a material assignment for this material if requested
    GLTFMaterialMeshList materialMeshList;
    if (_generateAssignments)
    {
        FilePath meshPath;
        unsigned int nodeCount = 0;
        unsigned int meshCount = 0;
        for (cgltf_size sceneIndex = 0; sceneIndex < data->scenes_count; ++sceneIndex)
        {
            cgltf_scene* scene = &data->scenes[sceneIndex];
            for (cgltf_size nodeIndex = 0; nodeIndex < scene->nodes_count; ++nodeIndex)
            {
                cgltf_node* cnode = scene->nodes[nodeIndex];
                if (!cnode)
                {
                    continue;
                }
                computeMeshMaterials(materialMeshList, cnode, meshPath, nodeCount, meshCount);
            }
        }

        // Add look with material assignments if requested
        LookPtr look = _materials->addLook();
        for (auto materialItem : materialMeshList)
        {
            const string& materialName = materialItem.first;
            const string& paths = materialItem.second;
  
            // Keep the assignment as simple as possible as more complex
            // systems such as USD can parse these files easily.
            MaterialAssignPtr matAssign = look->addMaterialAssign(EMPTY_STRING, materialName);
            matAssign->setGeom(paths);
        }
    }
}



MATERIALX_NAMESPACE_END
