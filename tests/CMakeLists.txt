set(TARGET_NAME ferrugo-core-tests)

set(UNIT_TEST_SOURCE_LIST
    container_utils.test.cpp
    ostream_utils.test.cpp
    iterator_interface.test.cpp
    pipe.test.cpp
    functional.test.cpp
    subrange.test.cpp
    chrono.test.cpp
)

Include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.4
)

FetchContent_MakeAvailable(Catch2)

add_executable(${TARGET_NAME} ${UNIT_TEST_SOURCE_LIST})
target_include_directories(
    ${TARGET_NAME}
    PUBLIC
    "${PROJECT_SOURCE_DIR}/include")

target_link_libraries(${TARGET_NAME} PRIVATE Catch2::Catch2WithMain)

add_test(
    NAME ${TARGET_NAME}
    COMMAND ${TARGET_NAME} -o report.xml -r junit)