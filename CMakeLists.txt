# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("binary-tools")

# Include sub-projects.
add_subdirectory ("injector")
add_subdirectory ("dll")
add_subdirectory ("idle-champions-example")
add_dependencies ("injector" "dll")
add_dependencies ("injector" "idle-champions-example")
