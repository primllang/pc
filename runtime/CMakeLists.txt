
add_compile_definitions(PRIMALC)

#disable C++ exceptions
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")

# Disable RTTI.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

# Address sanitizer
#    message("....... Enabling santizer on pcrt")
#    add_compile_options(-fsanitize=address)
#    add_link_options(-fsanitize=address)

add_subdirectory(pcrt)
add_subdirectory(test)