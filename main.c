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
课程内部不允许直接写文件,保证"统一先入内存".

动画由统一循环驱动,不再分成"终端循环"和"文件循环"两套代码:
循环只负责"在什么时刻渲染",真正把画面交给屏幕还是文件,
由 output_frame 这个回调按模式决定.

新增两种面向 shell 的入口:
- meta <lesson>:打印 "duration fps",供 shell 读取动画参数;
- <lesson> stream:把每一帧 PPM 连续写到 stdout,
  由 shell 管道直接送给 ffmpeg 生成 mp4/gif.
*/

typedef enum
{
    OUTPUT_ANSI,
    OUTPUT_PPM,
    OUTPUT_PPM_STREAM
} output_mode_t;

/*
输出回调:把"内存里已经画好的一帧"交给终端或 PPM 文件.
frame_index 只用于给动画帧命名(lessonXX_0000.ppm),不参与动画逻辑.
*/
static void output_frame(const lesson_t *lesson, const framebuffer_t *fb, int frame_index, output_mode_t mode)
{
    if (mode == OUTPUT_PPM)
    {
        /* 动画按帧号写序列文件,静态课程直接写单个文件 */
        char path[256];
        if (lesson->duration > 0.0f)
            snprintf(path, sizeof(path), "out/%s_%04d.ppm", lesson->id, frame_index);
        else
            snprintf(path, sizeof(path), "out/%s.ppm", lesson->id);
        output_ppm(fb, path);
        printf("已写出 %s\n", path);
    }
    else if (mode == OUTPUT_PPM_STREAM)
    {
        /* 帧流模式:纯数据走 stdout,进度提示走 stderr,不污染管道 */
        output_ppm_stream(fb);
    }
    else
    {
        /* 终端动画:先清屏再整幅打印;按帧率睡一觉,控制播放速度 */
        output_clear_screen();
        output_ansi(fb);
        if (lesson->duration > 0.0f)
            usleep((useconds_t)(1000000.0f / lesson->fps));
    }
}

/*
渲染进度条:输出到 stderr,不占用 stdout.
终端播放本身就能看到画面,不需要进度条;帧流/写文件时才有用.
*/
static void print_progress(int done, int total)
{
    const int bar_width = 30;
    int filled = (int)((long long)done * bar_width / total);
    int percent = (int)((long long)done * 100 / total);

    fprintf(stderr, "\r[");
    for (int i = 0; i < bar_width; i++)
        fputc(i < filled ? '#' : '-', stderr);
    fprintf(stderr, "] %3d%% %d/%d", percent, done, total);
    fflush(stderr);
}

/*
统一动画循环:
- 静态课程(duration=0):只渲染 t=0 这一帧;
- 动画课程:按 fps 把 [0, duration) 切成等间隔的时刻,
  每个时刻调用一次 render,再把结果交给 output_frame.
*/
static void run_lesson(const lesson_t *lesson, output_mode_t mode)
{
    /* PPM 输出目录统一放在 out/ 下,已被 .gitignore 忽略 */
    if (mode == OUTPUT_PPM)
        mkdir("out", 0777);

    if (lesson->duration <= 0.0f)
    {
        framebuffer_t fb;
        fb_clear(&fb);
        if (lesson->render(&fb, 0.0f) == 0)
            output_frame(lesson, &fb, 0, mode);
        return;
    }

    /* 总帧数 = 时长 * 帧率;dt 是"每两帧之间隔多少秒" */
    int total_frames = (int)(lesson->duration * lesson->fps + 0.5f);
    float dt = 1.0f / lesson->fps;

    if (mode == OUTPUT_PPM_STREAM)
        fprintf(stderr, "渲染 %s:共 %d 帧,正在写入标准输出...\n", lesson->id, total_frames);

    for (int frame_index = 0; frame_index < total_frames; frame_index++)
    {
        float time_seconds = (float)frame_index * dt;

        framebuffer_t fb;
        fb_clear(&fb);
        if (lesson->render(&fb, time_seconds) != 0)
            break;
        output_frame(lesson, &fb, frame_index, mode);

        if (mode == OUTPUT_PPM_STREAM || mode == OUTPUT_PPM)
            print_progress(frame_index + 1, total_frames);
    }

    if (mode == OUTPUT_PPM_STREAM || mode == OUTPUT_PPM)
        fprintf(stderr, "\n");
}

int main(int argc, char **argv)
{
    /* 无参数:运行默认课程 */
    if (argc < 2)
    {
        run_lesson(lesson_default(), OUTPUT_ANSI);
        return 0;
    }

    /* list:列出所有课程 */
    if (strcmp(argv[1], "list") == 0)
    {
        printf("可用课程:\n");
        lesson_list();
        return 0;
    }

    /* meta:机器可读的课程参数,shell 脚本据此配置 ffmpeg */
    if (strcmp(argv[1], "meta") == 0)
    {
        if (argc < 3)
        {
            fprintf(stderr, "用法: ray_tracing meta <lesson>\n");
            return 1;
        }
        const lesson_t *lesson = lesson_find(argv[2]);
        if (!lesson)
        {
            fprintf(stderr, "找不到课程 %s\n", argv[2]);
            return 1;
        }
        printf("%g %g\n", lesson->duration, lesson->fps);
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
        run_lesson(lesson, OUTPUT_PPM);
    /* 第二个参数写 stream:每一帧 PPM 写到 stdout,管道给 ffmpeg */
    else if (argc >= 3 && strcmp(argv[2], "stream") == 0)
        run_lesson(lesson, OUTPUT_PPM_STREAM);
    else
        run_lesson(lesson, OUTPUT_ANSI);

    return 0;
}
