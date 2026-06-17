# Downstream consumer project — built by following ONLY the .claude/skills/vsg guidance.
# This is the dogfood test: a fresh, external native VSG app using find_package(vsg).
cmake_minimum_required(VERSION 3.14)
project(vsg_consumer_test CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(vsg REQUIRED)

add_executable(vsg_consumer_test main.cpp)
target_link_libraries(vsg_consumer_test vsg::vsg)
