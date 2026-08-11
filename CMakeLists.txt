# 2026.03.19.08:12
cmake_minimum_required(VERSION 3.31)
project(matrix C)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable(matrix 
src/main.c 
src/matrix.c 
src/show.c
src/ray_tracing.c
)
target_link_libraries(matrix PRIVATE m)
# ok
