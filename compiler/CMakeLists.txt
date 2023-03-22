
add_executable(pc
    primalc.cpp
    token.cpp
    symtable.cpp
    entity.cpp
    codegen.cpp
    unit.cpp
    misc.cpp
    source.cpp
    resolve.cpp
    compstate.cpp
)
target_link_libraries(pc
    libbase
)
target_include_directories(pc
    PUBLIC include
)

#Address sanitizer
#   message("....... Enabling santizer on primalc")
#   add_compile_options(-fsanitize=address)
#   add_link_options(-fsanitize=address)
