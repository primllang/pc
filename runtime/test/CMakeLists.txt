
add_compile_definitions(PRIMALC)

#disable C++ exceptions
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")

# Disable RTTI.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

# Enable pthreads
IF(${CMAKE_HOST_LINUX})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
ENDIF()

# Address sanitizer
#    message("....... Enabling santizer on pcrt")
#    add_compile_options(-fsanitize=address)
#    add_link_options(-fsanitize=address)

add_executable(helloworld
    helloworld/helloworld.cpp
)
target_link_libraries(helloworld pcrt)

add_executable(prplayg
    prplayg/prplayg.cpp
)
target_link_libraries(prplayg pcrt)

add_executable(rttest
    rttest/arrtest.cpp
    rttest/reftest.cpp
)
target_link_libraries(rttest pcrt)

set_target_properties(rttest PROPERTIES COMPILE_FLAGS "-save-temps")