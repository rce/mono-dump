# CMakeList.txt : CMake project for injector, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

#find_library (PSAPI Psapi)
add_executable (injector "injector.cpp" "injector.h")
#target_link_libraries(injector -lpsapi)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET injector PROPERTY CXX_STANDARD 20)
endif()

set_target_properties(injector PROPERTIES LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\" /SUBSYSTEM:CONSOLE")

# TODO: Add tests and install targets if needed.
