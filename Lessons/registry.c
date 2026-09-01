#include "lesson.h"

#include <stdio.h>
#include <string.h>

#include "Lesson01_ColorGradient/lesson01.h"
#include "Lesson02_CubeWireframe/lesson02.h"
#include "Lesson03_RayTracedCube/lesson03.h"
#include "Lesson04_Sphere/lesson04.h"
#include "Lesson05_LightingShadows/lesson05.h"
#include "Lesson06_MultipleSpheres/lesson06.h"
#include "Lesson07_Reflection/lesson07.h"
#include "Lesson08_OrbitCamera/lesson08.h"
#include "Lesson09_Supersampling/lesson09.h"
#include "Lesson10_Multithreading/lesson10.h"
#include "Lesson11_TextureEarth/lesson11.h"

/*
课程注册表:
每节课登记 id/标题/是否动画和渲染函数.
animated=0 表示静态课程(只画一帧),animated=1 表示动画课程;
动画循环帧数由 lesson.h 里的全局 ANIMATION_FRAMES 统一决定,不在这里单独设速度.
新课程只需要在这里加一行,main 不需要改动.
*/
static const lesson_t lessons[] = {
    {"lesson01", "01 颜色渐变:方向映射颜色", 0, lesson01_render},
    {"lesson02", "02 立方体线框:矩阵变换 + 画线", 1, lesson02_render},
    {"lesson03", "03 光线追踪立方体:三角形求交", 1, lesson03_render},
    {"lesson04", "04 球体:二次方程求交", 0, lesson04_render},
    {"lesson05", "05 光照与阴影:漫反射", 0, lesson05_render},
    {"lesson06", "06 多球 + 镜面高光:Blinn-Phong", 0, lesson06_render},
    {"lesson07", "07 平面棋盘格 + 反射递归", 0, lesson07_render},
    {"lesson08", "08 轨道相机 + 透视投影", 1, lesson08_render},
    {"lesson09", "09 超采样抗锯齿(左右对比)", 0, lesson09_render},
    {"lesson10", "10 多线程渲染(支线)", 0, lesson10_render},
    {"lesson11", "11 贴图/材质:世界地图地球(支线)", 1, lesson11_render},
};
#define LESSON_COUNT (int)(sizeof(lessons) / sizeof(lessons[0]))

const lesson_t *lesson_find(const char *id)
{
    for (int i = 0; i < LESSON_COUNT; i++)
    {
        if (strcmp(lessons[i].id, id) == 0)
            return &lessons[i];
    }
    return NULL;
}

// 启动默认课程
const lesson_t *lesson_default(void)
{
    return &lessons[LESSON_COUNT - 1];
}

// 打印课程表
void lesson_list(void)
{
    for (int i = 0; i < LESSON_COUNT; i++)
    {
        printf("%-10s %s\n", lessons[i].id, lessons[i].title);
    }
}
