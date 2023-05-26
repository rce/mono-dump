# CMakeList.txt : CMake project for injector, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.
add_library (idle-champions-example MODULE "main.cpp" "monodump.h" "../dll-common/mono.cpp" "../dll-common/mono.h" "../dll-common/common.h" "../dll-common/InjectionLock.cpp" "../dll-common/InjectionLock.h")
SET(MONO_INCLUDE_DIRS "C:/Program Files/Mono/include/mono-2.0")
target_include_directories(idle-champions-example PUBLIC ${MONO_INCLUDE_DIRS})

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET idle-champions-example PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.