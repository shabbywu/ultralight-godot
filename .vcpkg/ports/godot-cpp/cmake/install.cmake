include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

get_target_property(GODOT_CPP_BINARY_DIR godot-cpp BINARY_DIR)
get_target_property(GODOT_CPP_SOURCE_DIR godot-cpp SOURCE_DIR)
get_target_property(GODOT_CPP_OUTPUT_NAME godot-cpp OUTPUT_NAME)

set_target_properties(
  godot-cpp
  PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES
    "$<BUILD_INTERFACE:${GODOT_CPP_SOURCE_DIR}/include>;$<BUILD_INTERFACE:${GODOT_CPP_BINARY_DIR}/gen/include>;$<BUILD_INTERFACE:${GODOT_CPP_SOURCE_DIR}/gdextension>;$<INSTALL_INTERFACE:include>"
    INTERFACE_SYSTEM_INCLUDE_DIRECTORIES
    "$<BUILD_INTERFACE:${GODOT_CPP_SOURCE_DIR}/include>;$<BUILD_INTERFACE:${GODOT_CPP_BINARY_DIR}/gen/include>;$<BUILD_INTERFACE:${GODOT_CPP_SOURCE_DIR}/gdextension>;$<INSTALL_INTERFACE:include>"
)

install(
  TARGETS godot-cpp
  EXPORT godot-cpp-targets
  DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY ${GODOT_CPP_SOURCE_DIR}/include/
                  ${GODOT_CPP_BINARY_DIR}/gen/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${GODOT_CPP_SOURCE_DIR}/gdextension/gdextension_interface.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${GODOT_CPP_SOURCE_DIR}/gdextension/extension_api.json
        DESTINATION ${CMAKE_INSTALL_DATADIR}/godot-cpp)

install(
  EXPORT godot-cpp-targets
  FILE godot-cpp-targets.cmake
  NAMESPACE godot-cpp::
  DESTINATION "${CMAKE_INSTALL_DATADIR}/godot-cpp")

file(READ "${GODOT_CPP_SOURCE_DIR}/gdextension/extension_api.json"
     GODOT_GDEXTENSION_API_JSON)
string(JSON GODOT_API_VERSION_MAJOR GET "${GODOT_GDEXTENSION_API_JSON}"
       "header" "version_major")
string(JSON GODOT_API_VERSION_MINOR GET "${GODOT_GDEXTENSION_API_JSON}"
       "header" "version_minor")
string(JSON GODOT_API_VERSION_PATCH GET "${GODOT_GDEXTENSION_API_JSON}"
       "header" "version_patch")
set(GODOT_API_VERSION
    "${GODOT_API_VERSION_MAJOR}.${GODOT_API_VERSION_MINOR}.${GODOT_API_VERSION_PATCH}"
)
unset(GODOT_GDEXTENSION_API_JSON)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/godot-cpp-config-version.cmake"
  VERSION "${GODOT_API_VERSION}"
  COMPATIBILITY SameMinorVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/godot-cpp-config-version.cmake"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/godot-cpp")
