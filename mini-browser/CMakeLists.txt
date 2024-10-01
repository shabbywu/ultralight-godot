find_package(PhysFS CONFIG REQUIRED)

include(bin2cpp.cmake)

# archive ui and embeded to cpp
execute_process(
  COMMAND ${CMAKE_COMMAND} -E tar c ${CMAKE_CURRENT_BINARY_DIR}/ui.zip
          --format=zip ${CMAKE_CURRENT_LIST_DIR}/assets/ui
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/assets)

execute_process(
  COMMAND ${Bin2cpp_executable} --file=${CMAKE_CURRENT_BINARY_DIR}/ui.zip
          --output=${CMAKE_CURRENT_SOURCE_DIR}/gen)

# archive inspector and embeded to cpp
get_target_property(ULTRALIGHT_INSPECTOR_DIR ultralight-sdk
                    ULTRALIGHT_INSPECTOR_DIR)
execute_process(
  COMMAND ${CMAKE_COMMAND} -E tar c ${CMAKE_CURRENT_BINARY_DIR}/inspector.zip
          --format=zip ${ULTRALIGHT_INSPECTOR_DIR}
  WORKING_DIRECTORY ${ULTRALIGHT_INSPECTOR_DIR}/..)

execute_process(
  COMMAND
    ${Bin2cpp_executable} --file=${CMAKE_CURRENT_BINARY_DIR}/inspector.zip
    --output=${CMAKE_CURRENT_SOURCE_DIR}/gen)

# Set the c++ standard to c++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB SRC_FILES src/*.cpp gen/*.cpp)
add_executable(mini-browser EXCLUDE_FROM_ALL ${SRC_FILES})
target_link_libraries(
  mini-browser ultralight-sdk-fullstack ulbind17::ulbind17
  $<IF:$<TARGET_EXISTS:PhysFS::PhysFS>,PhysFS::PhysFS,PhysFS::PhysFS-static>)
target_include_directories(mini-browser PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
