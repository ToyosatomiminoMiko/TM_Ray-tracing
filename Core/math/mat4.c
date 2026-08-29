/*
提供 4x4 矩阵乘法/向量乘法
3D 平移/旋转/缩放
透视投影/观察矩阵
矩阵打印方法
*/
#include "mat4.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

void matrix_identity_4x4(mat4 m)
{
    mat4 result = {
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1}};
    memcpy(m, result, sizeof(mat4));
}

void matrix_multiply_4x4(mat4 result, const mat4 a, const mat4 b)
{
    mat4 tmp;
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            float sum = 0;
            for (int k = 0; k < 4; ++k)
            {
                sum += a[i][k] * b[k][j];
            }
            tmp[i][j] = sum;
        }
    }
    memcpy(result, tmp, sizeof(mat4));
}

void matrix_vector_multiply(vec4 result, const mat4 matrix, const vec4 vector)
{
    vec4 tmp;
    for (int i = 0; i < 4; ++i)
    {
        tmp[i] = 0;
        for (int k = 0; k < 4; ++k)
        {
            tmp[i] += matrix[i][k] * vector[k];
        }
    }
    memcpy(result, tmp, sizeof(vec4));
}

void matrix_transpose_4x4(mat4 result, const mat4 m)
{
    mat4 tmp;
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            tmp[i][j] = m[j][i];
        }
    }
    memcpy(result, tmp, sizeof(mat4));
}

void translate_3d(mat4 homogeneous, float tx, float ty, float tz)
{
    mat4 result = {
        {1, 0, 0, tx},
        {0, 1, 0, ty},
        {0, 0, 1, tz},
        {0, 0, 0, 1}};
    memcpy(homogeneous, result, sizeof(mat4));
}

void rotate_x_3d(mat4 homogeneous, float theta)
{
    float c = cosf(theta);
    float s = sinf(theta);
    mat4 result = {
        {1, 0, 0, 0},
        {0, c, -s, 0},
        {0, s, c, 0},
        {0, 0, 0, 1}};
    memcpy(homogeneous, result, sizeof(mat4));
}

void rotate_y_3d(mat4 homogeneous, float theta)
{
    float c = cosf(theta);
    float s = sinf(theta);
    mat4 result = {
        {c, 0, s, 0},
        {0, 1, 0, 0},
        {-s, 0, c, 0},
        {0, 0, 0, 1}};
    memcpy(homogeneous, result, sizeof(mat4));
}

void rotate_z_3d(mat4 homogeneous, float theta)
{
    float c = cosf(theta);
    float s = sinf(theta);
    mat4 result = {
        {c, -s, 0, 0},
        {s, c, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1}};
    memcpy(homogeneous, result, sizeof(mat4));
}

void scale_3d(mat4 homogeneous, float sx, float sy, float sz)
{
    mat4 result = {
        {sx, 0, 0, 0},
        {0, sy, 0, 0},
        {0, 0, sz, 0},
        {0, 0, 0, 1}};
    memcpy(homogeneous, result, sizeof(mat4));
}

/*
绕自身轴 ZYX 旋转:矩阵列向量是旋转后的基向量
R = Rz(psi) * Ry(theta) * Rx(phi) 按教科书习惯排列
*/
void rotate_zyx_3d(mat4 homogeneous, float phi, float theta, float psi)
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

void perspective_4x4(mat4 m, float fov_radians, float aspect, float near, float far)
{
    float f = 1.0f / tanf(fov_radians * 0.5f);
    float nf = 1.0f / (near - far);
    mat4 result = {
        {f / aspect, 0, 0, 0},
        {0, f, 0, 0},
        {0, 0, (far + near) * nf, 2.0f * far * near * nf},
        {0, 0, -1, 0}};
    memcpy(m, result, sizeof(mat4));
}

void look_at_4x4(mat4 m, vec3 eye, vec3 center, vec3 up)
{
    vec3 f = vec3_normalize(vec3_sub(center, eye));
    vec3 s = vec3_normalize(vec3_cross(f, up));
    vec3 u = vec3_cross(s, f);

    mat4 result = {
        {s.x, s.y, s.z, -vec3_dot(s, eye)},
        {u.x, u.y, u.z, -vec3_dot(u, eye)},
        {-f.x, -f.y, -f.z, vec3_dot(f, eye)},
        {0, 0, 0, 1}};
    memcpy(m, result, sizeof(mat4));
}

void print_matrix(const mat4 matrix, int x, int y)
{
    for (int row = 0; row < x; row++)
    {
        for (int col = 0; col < y; col++)
        {
            printf("%8.4f ", matrix[row][col]);
        }
        printf("\n");
    }
}
