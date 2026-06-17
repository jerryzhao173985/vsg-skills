cmake_minimum_required(VERSION 3.14)
project(vsg_examples CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Builds the documented example programs against the installed VulkanSceneGraph.
# Each example is a complete, runnable app; building them is the compile/link gate
# that proves references/examples/*.md ground in the real API.
find_package(vsg REQUIRED)

foreach(example hello_vsg view_model builder_primitives custom_geometry)
    add_executable(${example} ${example}.cpp)
    target_link_libraries(${example} vsg::vsg)
endforeach()
