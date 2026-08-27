# 2026.03.19.08:12
cmake_minimum_required(VERSION 3.31)
project(matrix C)

include_directories(${PROJECT_SOURCE_DIR}/Inc)

add_executable(matrix
    Src/main.c
    Src/vector.c
    Src/matrix.c
    Src/show.c
    Src/ray_tracing.c
)
target_link_libraries(matrix PRIVATE m)
