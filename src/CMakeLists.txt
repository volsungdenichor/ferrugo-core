set(TARGET_NAME ferrugo-core)

add_executable(${TARGET_NAME} main.cpp)

target_include_directories(${TARGET_NAME} PRIVATE
    "${PROJECT_SOURCE_DIR}/include"
    "${zx_SOURCE_DIR}/include"
)

target_link_libraries(${TARGET_NAME} PRIVATE
    zx::ansi
)