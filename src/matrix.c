#include "matrix.h"

void matrix_multiply_4x4(Matrix_4x4 result, const Matrix_4x4 a, const Matrix_4x4 b)
{
    Matrix_4x4 tmp;
    for (uint8_t i = 0; i < 4; ++i)
    {
        for (uint8_t j = 0; j < 4; ++j)
        {
            float sum = 0;
            for (uint8_t k = 0; k < 4; ++k)
            {
                sum += a[i][k] * b[k][j];
            }
            tmp[i][j] = sum;
        }
    }
    memcpy(result, tmp, sizeof(Matrix_4x4));
}

void matrix_vector_multiply(Vector_4 result, const Matrix_4x4 matrix, const Vector_4 vector)
{
    Vector_4 tmp;
    for (uint8_t i = 0; i < 4; ++i)
    {
        tmp[i] = 0;
        for (uint8_t k = 0; k < 4; ++k)
        {
            tmp[i] += matrix[i][k] * vector[k];
        }
    }
    memcpy(result, tmp, sizeof(Vector_4));
}

void translate_3d(Matrix_4x4 homogeneous, float const tx, float const ty, float const tz)
{
    Matrix_4x4 result =
        {
            {1, 0, 0, tx},
            {0, 1, 0, ty},
            {0, 0, 1, tz},
            {0, 0, 0, 1}};
    memcpy(homogeneous, result, sizeof(Matrix_4x4));
}

void rotate_x_3d(Matrix_4x4 homogeneous, const float theta)
{
    float c = cosf(theta);
    float s = sinf(theta);
    Matrix_4x4 result =
        {
            {1, 0, 0, 0},
            {0, c, -s, 0},
            {0, s, c, 0},
            {0, 0, 0, 1}};
    memcpy(homogeneous, result, sizeof(Matrix_4x4));
}

void rotate_y_3d(Matrix_4x4 homogeneous, const float theta)
{
    float c = cosf(theta);
    float s = sinf(theta);
    Matrix_4x4 result =
        {
            {c, 0, s, 0},
            {0, 1, 0, 0},
            {-s, 0, c, 0},
            {0, 0, 0, 1}};
    memcpy(homogeneous, result, sizeof(Matrix_4x4));
}

void rotate_z_3d(Matrix_4x4 homogeneous, const float theta)
{
    float c = cosf(theta);
    float s = sinf(theta);
    Matrix_4x4 result =
        {
            {c, -s, 0, 0},
            {s, c, 0, 0},
            {0, 0, 1, 0},
            {0, 0, 0, 1}};
    memcpy(homogeneous, result, sizeof(Matrix_4x4));
}

void scale_3d(Matrix_4x4 homogeneous, float sx, float sy, float sz)
{
    Matrix_4x4 result =
        {
            {sx, 0, 0, 0},
            {0, sy, 0, 0},
            {0, 0, sz, 0},
            {0, 0, 0, 1}};
    memcpy(homogeneous, result, sizeof(Matrix_4x4));
}

void rotate_xyz_3d(Matrix_4x4 homogeneous, float phi, float theta, float psi)
{
    float cx = cosf(phi), sx = sinf(phi);
    float cy = cosf(theta), sy = sinf(theta);
    float cz = cosf(psi), sz = sinf(psi);

    homogeneous[0][0] = cy * cz;
    homogeneous[0][1] = sx * sy * cz - cx * sz;
    homogeneous[0][2] = cx * sy * cz + sx * sz;
    homogeneous[0][3] = 0;

    homogeneous[1][0] = cy * sz;
    homogeneous[1][1] = sx * sy * sz + cx * cz;
    homogeneous[1][2] = cx * sy * sz - sx * cz;
    homogeneous[1][3] = 0;

    homogeneous[2][0] = -sy;
    homogeneous[2][1] = sx * cy;
    homogeneous[2][2] = cx * cy;
    homogeneous[2][3] = 0;

    homogeneous[3][0] = 0;
    homogeneous[3][1] = 0;
    homogeneous[3][2] = 0;
    homogeneous[3][3] = 1;
}

void print_matrix(Matrix_4x4 matrix, const uint8_t x, const uint8_t y)
{
    for (uint8_t m = 0; m < x; m++)
    {
        for (uint8_t n = 0; n < y; n++)
        {
            printf("%8.4f ", matrix[m][n]);
        }
        printf("\r\n");
    }
}
