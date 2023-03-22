#NOTE: This file must match runtime\baseunit\CMakeLists.txt

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

