#include "ray_tracing.h"

// 向量归一化 把长度变成 1
Vector_3 vector3_normalize(Vector_3 v)
{
    float len = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    // 防止除以 0
    if (len == 0)
        return (Vector_3){0, 0, 0};
    return (Vector_3){v.x / len, v.y / len, v.z / len};
}

// 根据像素坐标生成光线方向
Vector_3 ray_direction(int i, int j)
{
    /*
    1.算 0..1 的像素中心
    屏幕中心是(0,0)
    这个像素在屏幕上的位置(百分比)
    */
    float u = (i + 0.5f) / (float)W;
    float v = (j + 0.5f) / (float)H;

    /*
    2.映射到 -1 到 +1
    0% -1, 50% 0, 100% 1
    */
    float x = 2.0f * u - 1.0f;
    float y = 2.0f * v - 1.0f;

    // 3.修正宽高比(让 X 轴适应屏幕宽度)
    float aspect_ratio = (float)W / (float)H;
    x = x * aspect_ratio;

    // 4.Y 轴取反(因为图像坐标 Y 向下)
    // 指上为天,指下为地
    y = -y;

    // 5.构造 3D 方向(看向 Z 负轴,屏幕放在 Z=-1)
    // 视角在原点位置
    // (屏幕上的X坐标, 屏幕上的Y坐标, 正前方的距离 -1)
    Vector_3 dir = {x, y, -1.0f};

    // 6.归一化
    // 只保留方向信息,实际空间长度均为1
    return vector3_normalize(dir);
}
