#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "lesson.h"
#include "output.h"

/*
统一渲染入口:
1. 每帧先清空内存帧缓冲
2. 调用课程渲染函数,把画面画进内存
3. 按模式从内存输出:终端 ANSI 动画,或 PPM 文件
课程内部不允许直接写文件,保证"统一先入内存"。
*/

static void run_ansi(const lesson_t *lesson)
{
    int frame_count = lesson->frames > 0 ? lesson->frames : 1;
    for (int frame = 0; frame < frame_count; frame++)
    {
        framebuffer_t fb;
        fb_clear(&fb);
        if (lesson->render(&fb, frame) != 0)
            break;

        output_clear_screen();
        output_ansi(&fb);
        if (lesson->frames > 0)
            usleep(lesson->frame_delay_us);
    }
}

static void run_ppm(const lesson_t *lesson)
{
    /* 输出目录统一放在 out/ 下,已被 .gitignore 忽略 */
    mkdir("out", 0777);

    int frame_count = lesson->frames > 0 ? lesson->frames : 1;
    for (int frame = 0; frame < frame_count; frame++)
    {
        framebuffer_t fb;
        fb_clear(&fb);
        if (lesson->render(&fb, frame) != 0)
            break;

        char path[256];
        if (lesson->frames > 0)
            snprintf(path, sizeof(path), "out/%s_%04d.ppm", lesson->id, frame);
        else
            snprintf(path, sizeof(path), "out/%s.ppm", lesson->id);

        output_ppm(&fb, path);
        printf("已写出 %s\n", path);
    }
}

int main(int argc, char **argv)
{
    /* 无参数:运行默认课程 */
    if (argc < 2)
    {
        run_ansi(lesson_default());
        return 0;
    }

    /* list:列出所有课程 */
    if (strcmp(argv[1], "list") == 0)
    {
        printf("可用课程:\n");
        lesson_list();
        return 0;
    }

    const lesson_t *lesson = lesson_find(argv[1]);
    if (!lesson)
    {
        printf("找不到课程 %s,可用课程:\n", argv[1]);
        lesson_list();
        return 1;
    }

    /* 第二个参数写 ppm:从内存帧缓冲输出 PPM 文件 */
    if (argc >= 3 && strcmp(argv[2], "ppm") == 0)
        run_ppm(lesson);
    else
        run_ansi(lesson);

    return 0;
}
