include(FetchContent)

set(ZX_BUILD_SEQUENCE ON CACHE BOOL "" FORCE)
set(ZX_BUILD_FUNCTIONAL ON CACHE BOOL "" FORCE)
set(ZX_BUILD_ANSI ON CACHE BOOL "" FORCE)

FetchContent_Declare(zx
    GIT_REPOSITORY git@github.com:volsungdenichor/zx.git
    GIT_TAG main
    EXCLUDE_FROM_ALL
    SYSTEM)

FetchContent_MakeAvailable(zx)

# include_directories(SYSTEM ${zx_SOURCE_DIR}/include)