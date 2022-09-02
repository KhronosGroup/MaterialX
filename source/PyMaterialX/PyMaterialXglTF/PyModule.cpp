#include <PyMaterialX/PyMaterialX.h>

namespace py = pybind11;

void bindPyCgltfMaterialHandler(py::module& mod);

PYBIND11_MODULE(PyMaterialXglTF, mod)
{
    mod.doc() = "Module containing Python bindings for the MaterialXglTFlibrary";

    bindPyCgltfMaterialHandler(mod);
}
