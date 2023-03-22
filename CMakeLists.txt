cmake_minimum_required(VERSION 3.24)

IF(${CMAKE_HOST_WIN32})
set(CMAKE_C_COMPILER "C:/Program Files/LLVM/bin/clang.exe")
set(CMAKE_CXX_COMPILER "C:/Program Files/LLVM/bin/clang++.exe")
ENDIF()

project(primalproj)

message(">>>> SYSTEM: ${CMAKE_SYSTEM_NAME}")

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/out)

add_subdirectory(libbase)
add_subdirectory(compiler)
add_subdirectory(runtime)

install(TARGETS pc)
install(DIRECTORY runtime/pcrt/include DESTINATION base/pcrt)
install(DIRECTORY runtime/pcrt/src DESTINATION base/pcrt)
install(DIRECTORY compiler/templates DESTINATION .)
install(FILES runtime/baseunit/CMakeLists.txt DESTINATION base/pcrt)
install(FILES runtime/baseunit/unit.yaml DESTINATION base)
install(DIRECTORY runtime/baseunit/src DESTINATION base)
