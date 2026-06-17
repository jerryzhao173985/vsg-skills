cmake_minimum_required(VERSION 3.14)
project(vsg_compile_probe CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Links the locally installed VulkanSceneGraph. vsgConfig.cmake pulls in its own
# find_dependency(Vulkan), which uses the VULKAN_SDK environment variable.
find_package(vsg REQUIRED)

add_executable(vsg_probe probe.cpp)
target_link_libraries(vsg_probe vsg::vsg)
