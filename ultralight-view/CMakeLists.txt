cmake_minimum_required(VERSION 3.13)
project(ultralight-view)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(TARGET_PATH x11)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(TARGET_PATH win64)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(TARGET_PATH macos-arm64)
else()
  message(FATAL_ERROR "Not implemented support for ${CMAKE_SYSTEM_NAME}")
endif()

# Change the output directory to the bin directory
set(BUILD_PATH ${CMAKE_SOURCE_DIR}/bin/${TARGET_PATH})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${BUILD_PATH}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${BUILD_PATH}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${BUILD_PATH}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${BUILD_PATH}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${BUILD_PATH}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${BUILD_PATH}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${BUILD_PATH}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${BUILD_PATH}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${BUILD_PATH}")

# Set the c++ standard to c++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(GODOT_COMPILE_FLAGS)
set(GODOT_LINKER_FLAGS)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  # using Visual Studio C++
  set(GODOT_COMPILE_FLAGS "${GODOT_COMPILE_FLAGS} /WX") # /GF /MP
  set(GODOT_COMPILE_FLAGS "${GODOT_COMPILE_FLAGS} /DTYPED_METHOD_BIND")

  if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(GODOT_COMPILE_FLAGS "${GODOT_COMPILE_FLAGS} /MDd") # /Od /RTC1 /Zi
  else()
    set(GODOT_COMPILE_FLAGS "${GODOT_COMPILE_FLAGS} /MD /O2") # /Oy /GL /Gy
    string(REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_CXX_FLAGS
                         "${CMAKE_CXX_FLAGS}")
    string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
  endif(CMAKE_BUILD_TYPE MATCHES Debug)

  # Disable conversion warning, truncation, unreferenced var, signed mismatch
  set(GODOT_COMPILE_FLAGS
      "${GODOT_COMPILE_FLAGS} /wd4244 /wd4305 /wd4101 /wd4018 /wd4267")

  add_definitions(-DNOMINMAX)

  # Unkomment for warning level 4 if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
  # string(REGEX REPLACE "/W[0-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  # endif()
else()
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(GODOT_LINKER_FLAGS "-static-libgcc -static-libstdc++")
  else()
    set(GODOT_LINKER_FLAGS "-framework Cocoa")
  endif()

  set(GODOT_COMPILE_FLAGS "-fPIC -g -Wwrite-strings")

  if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(GODOT_COMPILE_FLAGS
        "${GODOT_COMPILE_FLAGS} -fno-omit-frame-pointer -O0")
  else()
    set(GODOT_COMPILE_FLAGS "${GODOT_COMPILE_FLAGS} -O3")
  endif(CMAKE_BUILD_TYPE MATCHES Debug)
endif()

# The bridge reports conversion and script failures through C++ exceptions.
option(GODOT_DISABLE_EXCEPTIONS "Force disabling exception handling code" OFF)
if(GODOT_DISABLE_EXCEPTIONS)
  message(FATAL_ERROR "Ultralight 2.x bridge headers require C++ exceptions; use GODOT_DISABLE_EXCEPTIONS=OFF")
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  set(GODOT_COMPILE_FLAGS "${GODOT_COMPILE_FLAGS} /EHsc /utf-8")
endif()

# Generate source from the docs file
find_package(Python3 3.8 REQUIRED COMPONENTS Interpreter)
set(SDK_SHADER_HEADERS "${ULTRALIGHT_GODOT_SDK_ROOT}/platform/shaders/generated/headers/glsl")
file(GLOB SDK_SHADER_INPUTS "${SDK_SHADER_HEADERS}/*.h")
set(GENERATED_SHADERS "${CMAKE_CURRENT_BINARY_DIR}/generated/SDKShaders.hpp")
add_custom_command(OUTPUT "${GENERATED_SHADERS}"
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/generate_sdk_shaders.py"
    "${SDK_SHADER_HEADERS}" "${GENERATED_SHADERS}"
  DEPENDS ${SDK_SHADER_INPUTS} "${CMAKE_CURRENT_LIST_DIR}/generate_sdk_shaders.py" VERBATIM)
file(GLOB DOC_XML CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/doc_classes/*.xml")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${DOC_XML})

if(GENERATE_TEMPLATE_GET_NODE)
  set(GENERATE_BINDING_PARAMETERS "True")
else()
  set(GENERATE_BINDING_PARAMETERS "False")
endif()

execute_process(
  COMMAND
    "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_LIST_DIR}/doc_classes/make_doc_source.py"
    "${CMAKE_CURRENT_LIST_DIR}/src/doc_data_registration.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/doc_classes"
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})

# Get Sources
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.c**)
file(GLOB_RECURSE HEADERS include/*.h**)

# Define our godot-cpp library
add_library(${PROJECT_NAME} SHARED ${SOURCES} ${HEADERS} "${GENERATED_SHADERS}")

target_include_directories(${PROJECT_NAME} SYSTEM
                           PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src ${CMAKE_CURRENT_BINARY_DIR}/generated)
target_link_libraries(${PROJECT_NAME} godot-cpp::godot-cpp ulbind17::ulbind17)

# Create the correct name (godot.os.build_type.system_bits) Synchronized with
# godot-cpp's CMakeLists.txt
set(BITS 32)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(BITS 64)
endif(CMAKE_SIZEOF_VOID_P EQUAL 8)

if(CMAKE_BUILD_TYPE MATCHES Debug)
  set(GODOT_CPP_BUILD_TYPE Debug)
else()
  set(GODOT_CPP_BUILD_TYPE Release)
endif()

string(TOLOWER ${CMAKE_SYSTEM_NAME} SYSTEM_NAME)
string(TOLOWER ${GODOT_CPP_BUILD_TYPE} BUILD_TYPE)

# Add the compile flags
set_property(
  TARGET ${PROJECT_NAME}
  APPEND_STRING
  PROPERTY COMPILE_FLAGS ${GODOT_COMPILE_FLAGS})
set_property(
  TARGET ${PROJECT_NAME}
  APPEND_STRING
  PROPERTY LINK_FLAGS ${GODOT_LINKER_FLAGS})

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
    set(GODOT_ARCHITECTURE_TYPE "x86_64")
  elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86")
    set(GODOT_ARCHITECTURE_TYPE "x86_32")
  elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "armv7")
    set(GODOT_ARCHITECTURE_TYPE "arm32")
  elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    set(GODOT_ARCHITECTURE_TYPE "arm64")
  else()
    message(STATUS "host processor architecture is unknown")
    set(GODOT_ARCHITECTURE_TYPE "unknown")
  endif()

  set(OUTPUT_NAME
      ${PROJECT_NAME}.linux.${BUILD_TYPE}.${GODOT_ARCHITECTURE_TYPE})
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  if(CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
    set(GODOT_ARCHITECTURE_TYPE "x86_64")
  else()
    set(GODOT_ARCHITECTURE_TYPE "x86_32")
  endif()

  set(OUTPUT_NAME
      ${PROJECT_NAME}.windows.${BUILD_TYPE}.${GODOT_ARCHITECTURE_TYPE})
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(GODOT_ARCHITECTURE_TYPE arm64)
  set(OUTPUT_NAME
      ${PROJECT_NAME}.macos.${BUILD_TYPE}.${GODOT_ARCHITECTURE_TYPE})
else()
  message(FATAL_ERROR "Not implemented support for ${CMAKE_SYSTEM_NAME}")
endif()

set_target_properties(
  ${PROJECT_NAME}
  PROPERTIES OUTPUT_NAME ${OUTPUT_NAME}
             BUILD_WITH_INSTALL_RPATH TRUE)

install(TARGETS ${PROJECT_NAME}
        DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lib)

ultralight_godot_stage_runtime(${PROJECT_NAME} FALSE)
