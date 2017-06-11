cmake_minimum_required(VERSION 3.8)

include(ExternalProject)
set_property(DIRECTORY PROPERTY EP_BASE "${CMAKE_CURRENT_BINARY_DIR}/ExternalProjects")

# Boost
ExternalProject_Add(
    boost URL https://dl.bintray.com/boostorg/release/1.64.0/source/boost_1_64_0.tar.gz
    CONFIGURE_COMMAND "" BUILD_COMMAND "" INSTALL_COMMAND ""
)
ExternalProject_Get_Property(boost SOURCE_DIR)
set(BOOST_SOURCE_DIR "${SOURCE_DIR}")

# GSL
ExternalProject_Add(
    gsl URL https://github.com/Microsoft/GSL/archive/1f82596e1dada0067712527262a3d561ad51ddac.zip
    CONFIGURE_COMMAND "" BUILD_COMMAND "" INSTALL_COMMAND ""
)
ExternalProject_Get_Property(gsl SOURCE_DIR)
set(GSL_SOURCE_DIR "${SOURCE_DIR}")

# DeferredPtr
ExternalProject_Add(
    deferredptr DEPENDS gsl
    URL https://github.com/hsutter/gcpp/archive/9da9af2b87d55b899f1476f2d24104cb9606e45c.zip
    CONFIGURE_COMMAND "" BUILD_COMMAND "" INSTALL_COMMAND ""
)
ExternalProject_Get_Property(deferredptr SOURCE_DIR)
set(DEFERREDPTR_SOURCE_DIR "${SOURCE_DIR}")

# Main
ExternalProject_Add(
    js_cpp_rs DEPENDS boost deferredptr
    DOWNLOAD_COMMAND "" SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src"
    CMAKE_ARGS "-DCMAKE_PREFIX_PATH=${BOOST_SOURCE_DIR}$<SEMICOLON>${GSL_SOURCE_DIR}$<SEMICOLON>${DEFERREDPTR_SOURCE_DIR}"
    INSTALL_COMMAND ""
)
ExternalProject_Add_Step(
    js_cpp_rs test DEPENDEES build
    WORKING_DIRECTORY "<BINARY_DIR>" COMMAND ctest -C $<CONFIG>
    ALWAYS 1
)
