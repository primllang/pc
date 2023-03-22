add_library(libbase
    src/parse.cpp
    src/sys.cpp
    src/util.cpp
    src/var.cpp
    src/persist.cpp

    include/cmdline.h
    include/parse.h
    include/sys.h
    include/util.h
    include/var.h
    include/persist.h
)
target_include_directories(libbase
    PUBLIC include
)

add_executable(ptest
    test/ptest/tests1.cpp
    test/ptest/tests2.cpp
)
target_link_libraries(ptest
    libbase
)
target_include_directories(ptest
    PUBLIC include
)

message("....... Enabling santizer on ptest")
add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

#set_target_properties(ptest PROPERTIES COMPILE_FLAGS "-save-temps")