#ifndef MATRIX_H_
#define MATRIX_H_
#include "main.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

typedef float Matrix_4x4[4][4];
typedef float Vector_4[4];
typedef Vector_4 box[8];

// multiply
void matrix_multiply_4x4(Matrix_4x4 result, const Matrix_4x4 a, const Matrix_4x4 b);
void matrix_vector_multiply(Vector_4 result, const Matrix_4x4 mat, const Vector_4 vec);

void translate_3d(Matrix_4x4 homogeneous, float const tx, float const ty, float const tz);
void rotate_x_3d(Matrix_4x4 homogeneous, const float theta);
void rotate_y_3d(Matrix_4x4 homogeneous, const float theta);
void rotate_z_3d(Matrix_4x4 homogeneous, const float theta);
void rotate_xyz_3d(Matrix_4x4 homogeneous, float phi, float theta, float psi);
void scale_3d(Matrix_4x4 homogeneous, float sx, float sy, float sz);

void print_matrix(Matrix_4x4 matrix, const uint8_t x, const uint8_t y);

#endif /* MATRIX_H_ */
