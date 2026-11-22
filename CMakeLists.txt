cmake_minimum_required(VERSION 3.16)
project(CodePath LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- raylib 5.5 (downloaded automatically the first time you configure)
include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
# downloaded as a zip, so Git does not need to be installed
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(raylib)

# ---- app
file(GLOB SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)
add_executable(CodePath ${SOURCES})
target_link_libraries(CodePath PRIVATE raylib)
if (MSVC)
    target_compile_options(CodePath PRIVATE /W4)
else()
    target_compile_options(CodePath PRIVATE -Wall -Wextra -Wno-missing-field-initializers)
endif()
if (APPLE)
    target_link_libraries(CodePath PRIVATE "-framework IOKit" "-framework Cocoa" "-framework OpenGL")
endif()

# copy fonts + lesson content next to the executable after every build
add_custom_command(TARGET CodePath POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:CodePath>/assets)
