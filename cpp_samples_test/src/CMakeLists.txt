cmake_minimum_required(VERSION 3.8)

find_package(Boost)
find_path(GSL_INCLUDE_DIR gsl/gsl)
find_path(DEFERREDPTR_INCLUDE_DIR deferred_heap.h)

add_executable(main main.cpp)
set_property(TARGET main PROPERTY CXX_STANDARD 14)
target_include_directories(main PRIVATE
    "${Boost_INCLUDE_DIRS}"
    "${GSL_INCLUDE_DIR}"
    "${DEFERREDPTR_INCLUDE_DIR}"
)

if(MSVC)
    # https://support.microsoft.com/en-us/help/143208/prb-using-stl-in-windows-program-can-cause-min-max-conflicts
    target_compile_definitions(main PRIVATE -DNOMINMAX)
endif()

enable_testing()
add_test(NAME test COMMAND main)
