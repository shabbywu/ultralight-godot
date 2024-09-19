cmake_minimum_required(VERSION 3.13)
project(ultralight-godot)

set(CPP_BINDINGS_PATH ${CMAKE_CURRENT_LIST_DIR}/externals/godot-cpp CACHE STRING "Path to C++ bindings")
set(GODOT_GDEXTENSION_DIR
    ${CPP_BINDINGS_PATH}/gdextension
    CACHE STRING "Path to GDExtension interface header directory")

add_subdirectory(externals/ulbind17)
target_link_libraries(ulbind17 INTERFACE ultralight-sdk)

add_subdirectory(externals/godot-cpp)
add_subdirectory(ultralight-view)
