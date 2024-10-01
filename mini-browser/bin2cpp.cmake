message(STATUS "downloading bin2cpp...")
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  file(
    DOWNLOAD
    https://github.com/end2endzone/bin2cpp/releases/download/3.0.0/bin2cpp-3.0.0-centos-x86_64.tar.gz
    ${CMAKE_CURRENT_BINARY_DIR}/bin2cpp.tar.gz
    SHOW_PROGRESS
    EXPECTED_MD5 a45bf06769302737a266129acf5e8c2f)
  set(BIN2CPP_ARCHIVE_FILE ${CMAKE_CURRENT_BINARY_DIR}/bin2cpp.tar.gz)
  set(Bin2cpp_executable
      ${CMAKE_CURRENT_BINARY_DIR}/bin2cpp-3.0.0-centos-x86_64/bin/bin2cpp)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  file(
    DOWNLOAD
    https://github.com/end2endzone/bin2cpp/releases/download/3.0.0/bin2cpp-3.0.0-win64.zip
    ${CMAKE_CURRENT_BINARY_DIR}/bin2cpp.zip
    SHOW_PROGRESS
    EXPECTED_MD5 7e47caa3ae062645130bb23ead87dead)
  set(BIN2CPP_ARCHIVE_FILE ${CMAKE_CURRENT_BINARY_DIR}/bin2cpp.zip)
  set(Bin2cpp_executable
      ${CMAKE_CURRENT_BINARY_DIR}/bin2cpp-3.0.0-win64/bin/bin2cpp.exe)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  file(
    DOWNLOAD
    https://github.com/end2endzone/bin2cpp/releases/download/3.0.0/bin2cpp-3.0.0-macos.catalina-x86_64.tar.gz
    ${CMAKE_CURRENT_BINARY_DIR}/bin2cpp.tar.gz
    SHOW_PROGRESS
    EXPECTED_MD5 07b4869f472d5603154afa7c84e23275)
  set(BIN2CPP_ARCHIVE_FILE ${CMAKE_CURRENT_BINARY_DIR}/bin2cpp.tar.gz)
  set(Bin2cpp_executable
      ${CMAKE_CURRENT_BINARY_DIR}/bin2cpp-3.0.0-macos.catalina-x86_64/bin/bin2cpp
  )
else()
  message(FATAL_ERROR "Not implemented support for ${CMAKE_SYSTEM_NAME}")
endif()

file(ARCHIVE_EXTRACT INPUT ${BIN2CPP_ARCHIVE_FILE} DESTINATION
     ${CMAKE_CURRENT_BINARY_DIR})
