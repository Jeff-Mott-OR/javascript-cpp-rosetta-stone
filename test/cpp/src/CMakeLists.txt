cmake_minimum_required(VERSION 3.8)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost COMPONENTS unit_test_framework)

find_path(GSL_INCLUDE_DIR gsl/gsl)
find_path(DEFERREDPTR_INCLUDE_DIR deferred_heap.h)

add_executable(main main.cpp)
set_property(TARGET main PROPERTY CXX_STANDARD 14)
target_link_libraries(main PRIVATE Boost::boost Boost::unit_test_framework)
target_include_directories(main PRIVATE
    "${GSL_INCLUDE_DIR}"
    "${DEFERREDPTR_INCLUDE_DIR}"
)

if(MSVC)
    # https://support.microsoft.com/en-us/help/143208/prb-using-stl-in-windows-program-can-cause-min-max-conflicts
    target_compile_definitions(main PRIVATE -DNOMINMAX)
endif()

enable_testing()
add_test(NAME test COMMAND main)
