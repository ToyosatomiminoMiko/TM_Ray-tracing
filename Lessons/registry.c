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

/*
课程注册表:
每节课登记 id/标题/动画时长/帧率和渲染函数.
duration=0 表示静态课程(只画一帧),动画课程只需填"多少秒"和"每秒几帧",
总帧数由 main 自动算出(总帧数 = duration * fps).
新课程只需要在这里加一行,main 不需要改动.
*/
static const lesson_t lessons[] = {
    {"lesson01", "01 颜色渐变:方向映射颜色", 0, 0, lesson01_render},
    {"lesson02", "02 立方体线框:矩阵变换 + 画线", 1.0f, 60.0f, lesson02_render},
    {"lesson03", "03 光线追踪立方体:三角形求交", 1.0f, 60.0f, lesson03_render},
    {"lesson04", "04 球体:二次方程求交", 0, 0, lesson04_render},
    {"lesson05", "05 光照与阴影:漫反射", 0, 0, lesson05_render},
    {"lesson06", "06 多球 + 镜面高光:Blinn-Phong", 0, 0, lesson06_render},
    {"lesson07", "07 平面棋盘格 + 反射递归", 0, 0, lesson07_render},
    {"lesson08", "08 轨道相机 + 透视投影", 3.0f, 40.0f, lesson08_render},
    {"lesson09", "09 超采样抗锯齿(左右对比)", 0, 0, lesson09_render},
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

const lesson_t *lesson_default(void)
{
    return &lessons[LESSON_COUNT - 1];
}

void lesson_list(void)
{
    for (int i = 0; i < LESSON_COUNT; i++)
    {
        printf("%-10s %s\n", lessons[i].id, lessons[i].title);
    }
}
