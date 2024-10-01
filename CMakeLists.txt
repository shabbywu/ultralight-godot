cmake_minimum_required(VERSION 3.13)

project(ultralight-godot)

add_subdirectory(externals/ulbind17)
target_link_libraries(ulbind17 INTERFACE ultralight-sdk)

find_package(godot-cpp CONFIG REQUIRED)
add_subdirectory(ultralight-view)
