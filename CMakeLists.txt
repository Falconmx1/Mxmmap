cmake_minimum_required(VERSION 3.15)

project(Mxmmap
    VERSION 0.1.0
    DESCRIPTION "Herramienta ligera para analizar memoria RAM en Windows"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Solo Windows
if(NOT WIN32)
    message(FATAL_ERROR "Mxmmap solo compila en Windows.")
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(Mxmmap
    src/main.cpp
)

target_include_directories(Mxmmap PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_definitions(Mxmmap PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

target_link_libraries(Mxmmap PRIVATE
    psapi
)

if(MSVC)
    target_compile_options(Mxmmap PRIVATE /W4 /permissive-)
else()
    target_compile_options(Mxmmap PRIVATE -Wall -Wextra)
endif()
