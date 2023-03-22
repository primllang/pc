cmake_minimum_required(VERSION 3.24)

IF(${CMAKE_HOST_WIN32})
set(CMAKE_C_COMPILER "C:/Program Files/LLVM/bin/clang.exe")
set(CMAKE_CXX_COMPILER "C:/Program Files/LLVM/bin/clang++.exe")
ENDIF()

project(primalc CXX)

message("Compiler platform: ${CMAKE_SYSTEM_NAME}")

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/out)

add_compile_definitions(PRIMALC)

# Disable C++ exceptions
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")

# Disable RTTI.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

# Enable pthreads
IF(${CMAKE_HOST_LINUX})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
ENDIF()

add_library(pcrt
    src/os.cpp
    src/mem.cpp
    src/strings.cpp
    src/arr.cpp

    include/plarr.h
    include/plbase.h
    include/plmem.h
    include/plobj.h
    include/plstr.h
)
target_include_directories(pcrt
    PUBLIC include
)

