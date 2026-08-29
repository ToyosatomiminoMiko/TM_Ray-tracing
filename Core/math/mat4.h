#ifndef MAT4_H_
#define MAT4_H_

#include "vec3.h"

/*
4x4 齐次矩阵,用于 3D 变换:
平移/旋转/缩放/透视投影/观察矩阵.
所有函数都是纯函数:只修改 result/homogeneous 这个输出参数.
*/
typedef float mat4[4][4];
typedef float vec4[4];
typedef vec4 box[8]; /* 8 个顶点组成的立方体 */

/* 单位矩阵 I */
void matrix_identity_4x4(mat4 m);

/*
矩阵乘法 C = A * B
c[i][j] = Σ(k=0..3) a[i][k] * b[k][j]
注意:矩阵乘法不满足交换律,A*B 表示"先做 B,再做 A".
*/
void matrix_multiply_4x4(mat4 result, const mat4 a, const mat4 b);

/* 矩阵乘向量: out = M * v */
void matrix_vector_multiply(vec4 result, const mat4 mat, const vec4 vec);

/* 矩阵转置: result[i][j] = m[j][i] (旋转矩阵的逆等于转置) */
void matrix_transpose_4x4(mat4 result, const mat4 m);

/*
平移矩阵:
| 1 0 0 tx |
| 0 1 0 ty |
| 0 0 1 tz |
| 0 0 0 1  |
*/
void translate_3d(mat4 homogeneous, float tx, float ty, float tz);

/*
绕 X 轴旋转 theta(弧度):
| 1  0   0  0 |
| 0  cos -sin 0 |
| 0  sin  cos 0 |
| 0  0   0   1 |
*/
void rotate_x_3d(mat4 homogeneous, float theta);

/*
绕 Y 轴旋转 theta(弧度):
| cos  0  sin 0 |
| 0    1  0   0 |
| -sin 0  cos 0 |
| 0    0  0   1 |
*/
void rotate_y_3d(mat4 homogeneous, float theta);

/*
绕 Z 轴旋转 theta(弧度):
| cos -sin 0 0 |
| sin  cos 0 0 |
| 0    0   1 0 |
| 0    0   0 1 |
*/
void rotate_z_3d(mat4 homogeneous, float theta);

/* 按 ZYX 顺序的复合旋转(先绕 Z? 见实现注释),输入弧度 */
void rotate_zyx_3d(mat4 homogeneous, float phi, float theta, float psi);

/*
缩放矩阵:
| sx 0  0  0 |
| 0  sy 0  0 |
| 0  0  sz 0 |
| 0  0  0  1 |
*/
void scale_3d(mat4 homogeneous, float sx, float sy, float sz);

/*
透视投影矩阵(OpenGL 风格,深度映射到 [-1,1]):
f = 1 / tan(fov/2)
| f/aspect 0   0   0 |
| 0        f   0   0 |
| 0        0   (far+near)/(near-far)  2*far*near/(near-far) |
| 0        0   -1  0 |
*/
void perspective_4x4(mat4 m, float fov_radians, float aspect, float near, float far);

/*
观察矩阵 LookAt:
根据眼睛位置/注视点和上方向构造相机坐标系.
f = normalize(center-eye)
s = normalize(f × up)
u = s × f
*/
void look_at_4x4(mat4 m, vec3 eye, vec3 center, vec3 up);

/* 打印矩阵(教学调试用) */
void print_matrix(const mat4 matrix, int x, int y);

#endif /* MAT4_H_ */
