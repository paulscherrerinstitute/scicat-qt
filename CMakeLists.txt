cmake_minimum_required(VERSION 3.5)

project(SciCat LANGUAGES CXX)

# build app
add_subdirectory(PSIArchiver)

# install resources
add_subdirectory(resources)