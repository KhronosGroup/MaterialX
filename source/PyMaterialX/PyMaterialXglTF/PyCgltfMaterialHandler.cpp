
#include <PyMaterialX/PyMaterialX.h>

#include <MaterialXglTF/CgltfMaterialHandler.h>

namespace py = pybind11;
namespace mx = MaterialX;

void bindPyCgltfMaterialHandler(py::module& mod)
{
    py::class_<mx::CgltfMaterialHandler, mx::CgltfMaterialHandlerPtr>(mod, "CgltfMaterialHandler")
        .def("load", &mx::CgltfMaterialHandler::load)
        .def("save", &mx::CgltfMaterialHandler::save)
        .def("extensionsSupported", &mx::CgltfMaterialHandler::extensionsSupported)
        .def("setDefinitions", &mx::CgltfMaterialHandler::setDefinitions)
        .def("setMaterials", &mx::CgltfMaterialHandler::setMaterials)
        .def("getMaterials", &mx::CgltfMaterialHandler::getMaterials)
        .def("setGenerateAssignments", &mx::CgltfMaterialHandler::setGenerateAssignments)
        .def("getGenerateAssignments", &mx::CgltfMaterialHandler::getGenerateAssignments)
        .def("setGenerateFullDefinitions", &mx::CgltfMaterialHandler::setGenerateFullDefinitions)
        .def("getGenerateFullDefinitions", &mx::CgltfMaterialHandler::getGenerateFullDefinitions);
}
