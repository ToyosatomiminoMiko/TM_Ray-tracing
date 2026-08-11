#include "matrix.h"
#include "show.h"
#include "ray_tracing.h"
#include "main.h"

/*
声明式绘图
*/
void line(float x0, float y0, float x1, float y1, pixel_color_t color)
{
    draw_line(
        (uint8_t)(x0 + W / 2), (uint8_t)(y0 + H / 2),
        (uint8_t)(x1 + W / 2), (uint8_t)(y1 + H / 2), color);
}

/*
(ANSI)clear
`\033[3J` 清除滚动缓冲区(scrollback buffer)
`\033[2J` 清空整个屏幕
`\033[H` 将光标移动到屏幕左上角
*/
void clear()
{
    printf("\033[3J\033[2J\033[H");
}

const box b1 = {
    {-1, -1, -1, 1},
    {1, -1, -1, 1},
    {1, -1, 1, 1},
    {-1, -1, 1, 1},
    {-1, 1, 1, 1},
    {-1, 1, -1, 1},
    {1, 1, -1, 1},
    {1, 1, 1, 1}};

void cube()
{
    //  变换结果储存
    box b2;
    Matrix_4x4 rot, s0;
    float r = 0;
    uint16_t n = 0;
    while (1)
    {
        Matrix_4x4 identity_matrix =
            {
                {1, 0, 0, 0},
                {0, 1, 0, 0},
                {0, 0, 1, 0},
                {0, 0, 0, 1}};
        if (n >= 360)
            n = 0;
        // transformation
        r = (M_PI / 180) * (float)n++;
        rotate_xyz_3d(rot, r, -r, r);
        scale_3d(s0, 15, 15, 15);
        // matrix multiply
        matrix_multiply_4x4(identity_matrix, rot, identity_matrix);
        matrix_multiply_4x4(identity_matrix, s0, identity_matrix);
        // vector multiply
        for (uint8_t i = 0; i < 8; i++)
        {
            Vector_4 temp_vec0 =
                {b1[i][0], b1[i][1], b1[i][2], 1};
            matrix_vector_multiply(temp_vec0, identity_matrix, temp_vec0);
            memcpy(b2[i], temp_vec0, sizeof(Vector_4));
        }
        // 底面边 - 红色
        line(b2[0][0], b2[0][1], b2[1][0], b2[1][1], COLOR_WHITE);
        line(b2[1][0], b2[1][1], b2[2][0], b2[2][1], COLOR_WHITE);
        line(b2[2][0], b2[2][1], b2[3][0], b2[3][1], COLOR_WHITE);
        line(b2[3][0], b2[3][1], b2[0][0], b2[0][1], COLOR_WHITE);

        // 垂直棱 - 绿色
        line(b2[3][0], b2[3][1], b2[4][0], b2[4][1], COLOR_PINK);
        line(b2[0][0], b2[0][1], b2[5][0], b2[5][1], COLOR_PINK);
        line(b2[1][0], b2[1][1], b2[6][0], b2[6][1], COLOR_PINK);
        line(b2[2][0], b2[2][1], b2[7][0], b2[7][1], COLOR_PINK);

        // 顶面边 - 蓝色
        line(b2[5][0], b2[5][1], b2[6][0], b2[6][1], COLOR_BLUE);
        line(b2[6][0], b2[6][1], b2[7][0], b2[7][1], COLOR_BLUE);
        line(b2[7][0], b2[7][1], b2[4][0], b2[4][1], COLOR_BLUE);
        line(b2[4][0], b2[4][1], b2[5][0], b2[5][1], COLOR_BLUE);
        // draw
        clear();
        graphmem_show();
        clear_gmem();
        print_matrix(identity_matrix, 4, 4);
        usleep(16383);
    }
}

// 返回值:1 相交,0 不相交；t 为距离,u,v 为重心坐标
int ray_triangle_intersect(
    Vector_3 O, Vector_3 D, Vector_3 V0, Vector_3 V1, Vector_3 V2,
    float *t, float *u, float *v)
{
    Vector_3 edge1 = {V1.x - V0.x, V1.y - V0.y, V1.z - V0.z};
    Vector_3 edge2 = {V2.x - V0.x, V2.y - V0.y, V2.z - V0.z};
    Vector_3 h, s, q;
    float a, f, inv_det;

    // 计算 h = D × edge2
    h.x = D.y * edge2.z - D.z * edge2.y;
    h.y = D.z * edge2.x - D.x * edge2.z;
    h.z = D.x * edge2.y - D.y * edge2.x;

    a = edge1.x * h.x + edge1.y * h.y + edge1.z * h.z;
    if (a > -1e-6 && a < 1e-6)
        return 0; // 平行或共面

    inv_det = 1.0f / a;
    s.x = O.x - V0.x;
    s.y = O.y - V0.y;
    s.z = O.z - V0.z;
    *u = inv_det * (s.x * h.x + s.y * h.y + s.z * h.z);
    if (*u < 0.0f || *u > 1.0f)
        return 0;

    q.x = s.y * edge1.z - s.z * edge1.y;
    q.y = s.z * edge1.x - s.x * edge1.z;
    q.z = s.x * edge1.y - s.y * edge1.x;
    *v = inv_det * (D.x * q.x + D.y * q.y + D.z * q.z);
    if (*v < 0.0f || *u + *v > 1.0f)
        return 0;

    *t = inv_det * (edge2.x * q.x + edge2.y * q.y + edge2.z * q.z);
    if (*t > 1e-6)
        return 1; // 确保交点在光线前方
    return 0;
}

void rt_cube()
{
    // 12 个三角形索引
    const uint8_t tri_indices[12][3] = {
        {0, 1, 2},
        {0, 2, 3}, // 底面
        {5, 6, 7},
        {5, 7, 4}, // 顶面
        {3, 2, 7},
        {3, 7, 4}, // 前面
        {0, 5, 6},
        {0, 6, 1}, // 后面
        {0, 3, 4},
        {0, 4, 5}, // 左面
        {1, 6, 7},
        {1, 7, 2} // 右面
    };

    box b2;
    Vector_3 origin = {0.0f, 0.0f, 0.0f};
    uint16_t n = 0;
    float r;
    Matrix_4x4 S, R, T;
    while (1)
    {
        // 构造模型矩阵:先缩放,再旋转,再平移
        Matrix_4x4 model = {
            {1, 0, 0, 0},
            {0, 1, 0, 0},
            {0, 0, 1, 0},
            {0, 0, 0, 1}};
        if (n >= 360)
            n = 0;
        r = (M_PI / 180.0f) * (float)n++;
        scale_3d(S, 15, 15, 15);    // 缩放
        rotate_xyz_3d(R, r, -r, r); // 旋转 绕自身轴
        translate_3d(T, 0, 0, -50); // 平移到相机前方

        // 矩阵乘法:model = T * R * S 顺序很重要
        matrix_multiply_4x4(model, S, model); // model = S
        matrix_multiply_4x4(model, R, model); // model = R*S
        matrix_multiply_4x4(model, T, model); // model = T*R*S

        // 变换顶点得到世界坐标
        for (uint8_t i = 0; i < 8; i++)
        {
            Vector_4 temp = {b1[i][0], b1[i][1], b1[i][2], 1};
            matrix_vector_multiply(temp, model, temp);
            memcpy(b2[i], temp, sizeof(Vector_4));
        }

        // 光线追踪渲染
        clear_gmem();
        for (int j = 0; j < H; j++)
        {
            for (int i = 0; i < W; i++)
            {
                Vector_3 dir = ray_direction(i, j);
                float t_min = 1e30f;
                int hit = 0;
                Vector_3 hit_normal = {0, 0, 0};

                for (int k = 0; k < 12; k++)
                {
                    int idx0 = tri_indices[k][0];
                    int idx1 = tri_indices[k][1];
                    int idx2 = tri_indices[k][2];
                    Vector_3 V0 = {b2[idx0][0], b2[idx0][1], b2[idx0][2]};
                    Vector_3 V1 = {b2[idx1][0], b2[idx1][1], b2[idx1][2]};
                    Vector_3 V2 = {b2[idx2][0], b2[idx2][1], b2[idx2][2]};

                    float t, u, v;
                    if (ray_triangle_intersect(origin, dir, V0, V1, V2, &t, &u, &v))
                    {
                        if (t > 0 && t < t_min)
                        {
                            t_min = t;
                            hit = 1;
                            // 面法线
                            Vector_3 edge1 = {V1.x - V0.x, V1.y - V0.y, V1.z - V0.z};
                            Vector_3 edge2 = {V2.x - V0.x, V2.y - V0.y, V2.z - V0.z};
                            hit_normal = vector3_normalize((Vector_3){
                                edge1.y * edge2.z - edge1.z * edge2.y,
                                edge1.z * edge2.x - edge1.x * edge2.z,
                                edge1.x * edge2.y - edge1.y * edge2.x});
                        }
                    }
                }

                int r, g, b;
                if (hit)
                {
                    r = (int)((hit_normal.x + 1.0f) * 0.5f * 255);
                    g = (int)((hit_normal.y + 1.0f) * 0.5f * 255);
                    b = (int)((hit_normal.z + 1.0f) * 0.5f * 255);
                }
                else
                {
                    // 背景(渐变天空)
                    // r = (int)((dir.y + 1.0f) * 0.5f * 255);
                    // g = (int)((dir.y + 1.0f) * 0.5f * 255);
                    // b = 255;
                    r = g = b = 0;
                }
                set_point_color(i, j, r << 16 | g << 8 | b);
            }
        }
        clear();
        graphmem_show();
        usleep(8191);
    }
}

void output()
{
    // 遍历每一行(从上到下)
    for (int j = 0; j < H; j++)
    {
        for (int i = 0; i < W; i++)
        {
            // 生成这束光的方向
            Vector_3 dir = ray_direction(i, j);
            // 把方向向量映射成 0..255 的颜色值
            // 如果方向朝右(x正)红色多,朝上(y正)绿色多
            int r = (int)((dir.x + 1.0f) * 0.5f * 255); // 把 -1..1 映射到 0..255
            int g = (int)((dir.y + 1.0f) * 0.5f * 255);
            int b = (int)((dir.z + 1.0f) * 0.5f * 255); // 由于看向 -Z,这里会偏暗蓝
            set_point_color(i, j, r << 16 | g << 8 | b);
        }
    }
    clear();
    graphmem_show();
}

// 性能优化版求交(利用了 D 归一化)
// 返回 t(距离),未命中返回 -1
float intersect_sphere_fast(Vector_3 O, Vector_3 D, Vector_3 C, float R)
{
    // 1. 计算从球心指向光线起点的向量(注意方向)
    Vector_3 oc = {O.x - C.x, O.y - C.y, O.z - C.z};
    // 2. 计算 b 的一半(点积)
    float b_half = D.x * oc.x + D.y * oc.y + D.z * oc.z;
    // 3. 计算 c
    float c = (oc.x * oc.x + oc.y * oc.y + oc.z * oc.z) - R * R;
    // 4. 判别式(优化掉乘法)
    float delta = b_half * b_half - c;
    if (delta < 0)
        return -1; // 光线擦肩而过
    // 5. 求最近的根(因为 a=1,所以这里只减一次)
    float sqrt_delta = sqrt(delta);
    float t = -b_half - sqrt_delta; // 负号对应最近的交点
    if (t < 0)
        return -1; // 球在相机背后
    return t;
}

// 传入光线起点 O(0,0,0),方向 D,球心 C,半径 R
// 返回 t 值(距离),如果没碰到返回 -1
float intersect_sphere_simple(Vector_3 O, Vector_3 D, Vector_3 C, float R)
{
    Vector_3 OC = {O.x - C.x, O.y - C.y, O.z - C.z}; // 指向球心的向量
    float a = 1.0;                                   // 因为D归一化
    float b = 2.0 * (D.x * OC.x + D.y * OC.y + D.z * OC.z);
    float c = (OC.x * OC.x + OC.y * OC.y + OC.z * OC.z) - R * R;
    float delta = b * b - 4 * a * c; // 判别式
    if (delta < 0)
        return -1;                            // 没碰到
    float t = (-b - sqrt(delta)) / (2.0 * a); // 取最近的交点(减号)
    if (t < 0)
        return -1; // 球在背后
    return t;
}

void ball()
{
    // 定义一个球:放在 (0, 0, -3) 处,半径 1.0
    Vector_3 sphere_center = {0.0f, 0.0f, -3.0f};
    float sphere_radius = 1.0f;
    Vector_3 origin = {0.0f, 0.0f, 0.0f};

    for (int j = 0; j < H; j++)
    {
        for (int i = 0; i < W; i++)
        {
            Vector_3 dir = ray_direction(i, j);

            // 调用性能优化版求交
            float t = intersect_sphere_fast(origin, dir, sphere_center, sphere_radius);
            // float t = intersect_sphere_simple(origin, dir, sphere_center, sphere_radius);
            int r, g, b;
            if (t > 0)
            {
                // 命中！计算命中点的位置
                Vector_3 hit_point = {origin.x + dir.x * t,
                                      origin.y + dir.y * t,
                                      origin.z + dir.z * t};
                // 计算法线(从球心指向命中点,并归一化)
                Vector_3 normal = {hit_point.x - sphere_center.x,
                                   hit_point.y - sphere_center.y,
                                   hit_point.z - sphere_center.z};
                normal = vector3_normalize(normal); // 法线长度变1

                // 把法线方向映射成颜色(像不像红蓝3D眼镜？)
                r = (int)((normal.x + 1.0f) * 0.5f * 255);
                g = (int)((normal.y + 1.0f) * 0.5f * 255);
                b = (int)((normal.z + 1.0f) * 0.5f * 255);
            }
            else
            {
                // 背景色(渐变天空)
                r = (int)((dir.y + 1.0f) * 0.5f * 255);
                g = (int)((dir.y + 1.0f) * 0.5f * 255);
                b = 255;
            }
            set_point_color(i, j, r << 16 | g << 8 | b);
        }
    }
    clear();
    graphmem_show();
}

int main()
{
    // output();
    // cube();
    rt_cube();
    // ball();
    return 0;
}
