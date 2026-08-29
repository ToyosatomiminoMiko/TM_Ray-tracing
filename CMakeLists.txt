# 2026.08.27 架构重构:Core 核心库 + Lessons 课程 + Docs 文档
cmake_minimum_required(VERSION 3.16)
project(ray_tracing C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS Core/*.c)
file(GLOB_RECURSE LESSON_SOURCES CONFIGURE_DEPENDS Lessons/*.c)

add_executable(ray_tracing
    main.c
    ${CORE_SOURCES}
    ${LESSON_SOURCES}
)

find_package(Threads REQUIRED)

target_include_directories(ray_tracing PRIVATE
    Core/math
    Core/render
    Core/raytrace
    Lessons
)

target_link_libraries(ray_tracing PRIVATE m Threads::Threads)
