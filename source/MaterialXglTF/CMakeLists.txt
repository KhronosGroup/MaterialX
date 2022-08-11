include_directories(
    ${EXTERNAL_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/../)

file(GLOB_RECURSE materialx_source "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE materialx_headers "${CMAKE_CURRENT_SOURCE_DIR}/*.h*")


assign_source_group("Source Files" ${materialx_source})
assign_source_group("Header Files" ${materialx_headers})

if(UNIX)
    add_compile_options(-Wno-unused-function)
endif()

add_library(MaterialXglTF ${materialx_source} ${materialx_headers})

add_definitions(-DMATERIALX_GLTF_EXPORTS)

target_link_libraries(
    MaterialXglTF
    MaterialXCore
    MaterialXFormat
    ${CMAKE_DL_LIBS})

set_target_properties(
    MaterialXglTF PROPERTIES
    OUTPUT_NAME MaterialXglTF${MATERIALX_LIBNAME_SUFFIX}
    COMPILE_FLAGS "${EXTERNAL_COMPILE_FLAGS}"
    LINK_FLAGS "${EXTERNAL_LINK_FLAGS}"
    VERSION "${MATERIALX_LIBRARY_VERSION}"
    SOVERSION "${MATERIALX_MAJOR_VERSION}")

install(TARGETS MaterialXglTF
    EXPORT MaterialX
    ARCHIVE DESTINATION ${MATERIALX_INSTALL_LIB_PATH}
    LIBRARY DESTINATION ${MATERIALX_INSTALL_LIB_PATH}
    RUNTIME DESTINATION bin)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/"
    DESTINATION ${MATERIALX_INSTALL_INCLUDE_PATH}/MaterialXglTF/ MESSAGE_NEVER
    FILES_MATCHING PATTERN "*.h*")

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/MaterialXglTF.pdb"
    DESTINATION "${MATERIALX_INSTALL_LIB_PATH}/" OPTIONAL)
