find_package(PhysFS CONFIG REQUIRED)

find_package(Python3 3.8 REQUIRED COMPONENTS Interpreter)
get_target_property(ULTRALIGHT_INSPECTOR_DIR ultralight-sdk ULTRALIGHT_INSPECTOR_DIR)
file(GLOB_RECURSE UI_INPUTS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/*")
file(GLOB_RECURSE INSPECTOR_INPUTS "${ULTRALIGHT_INSPECTOR_DIR}/*")
set(GENERATED_ASSETS "${CMAKE_CURRENT_BINARY_DIR}/generated_assets")
add_custom_command(OUTPUT "${GENERATED_ASSETS}/ui.cpp" "${GENERATED_ASSETS}/ui.h"
    "${GENERATED_ASSETS}/inspector.cpp" "${GENERATED_ASSETS}/inspector.h" "${GENERATED_ASSETS}/archive.h"
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/embed_assets.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui" "${ULTRALIGHT_INSPECTOR_DIR}" "${GENERATED_ASSETS}"
  DEPENDS ${UI_INPUTS} ${INSPECTOR_INPUTS} "${CMAKE_CURRENT_SOURCE_DIR}/embed_assets.py" VERBATIM)

# Set the c++ standard to c++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(mini-browser EXCLUDE_FROM_ALL
  src/main.cpp src/Browser.cpp src/Page.cpp src/UI.cpp
  "${GENERATED_ASSETS}/ui.cpp" "${GENERATED_ASSETS}/inspector.cpp")
target_link_libraries(
  mini-browser PRIVATE ultralight-sdk-fullstack ulbind17::ulbind17
  $<IF:$<TARGET_EXISTS:PhysFS::PhysFS>,PhysFS::PhysFS,PhysFS::PhysFS-static>)
target_include_directories(mini-browser PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(TARGET_PATH x11)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(TARGET_PATH win64)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(TARGET_PATH macos-arm64)
else()
  message(FATAL_ERROR "Not implemented support for ${CMAKE_SYSTEM_NAME}")
endif()

set_target_properties(
  mini-browser PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                          ${CMAKE_SOURCE_DIR}/bin/mini-browser/${TARGET_PATH})

ultralight_godot_stage_runtime(mini-browser TRUE)
