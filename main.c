#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "lesson.h"
#include "output.h"

/*
统一渲染入口:
1. 每帧先清空内存帧缓冲
2. 调用课程渲染函数,把画面画进内存
3. 按模式从内存输出:PPM 文件,或连续 PPM 帧流(管道给 ffmpeg)
课程内部不允许直接写文件,保证"统一先入内存".

动画由统一循环驱动,不再按输出方式拆成两套循环:
循环只负责"在什么时刻渲染",真正把画面交给文件还是管道,
由 output_frame 这个回调按模式决定.
可用的输出只有 ppm / mp4 / gif.

动画是"帧驱动"而不是"时间驱动":
课程只标记自己是不是动画,循环帧数由全局 ANIMATION_FRAMES 统一决定,
main 依次渲染第 0..ANIMATION_FRAMES-1 帧,
渲染函数收到帧号,由帧号决定画面,和秒/帧率无关.
所有动画课程共用同一个速度,想调快/调慢只改 lesson.h 里的 ANIMATION_FRAMES.
播放帧率只属于编码端:MP4 默认 60fps,GIF 锁定 25fps,
这两个数字由 ray.sh 决定,不会倒回来影响动画本身.

新增两种面向 shell 的入口:
- meta <lesson>:打印课程帧数,供 shell 读取动画参数;
- <lesson> stream:把每一帧 PPM(P6 二进制)连续写到 stdout,
  由 shell 管道直接送给 ffmpeg 生成 mp4/gif.
*/

typedef enum
{
    OUTPUT_PPM,
    OUTPUT_PPM_STREAM
} output_mode_t;

/*
输出回调:把"内存里已经画好的一帧"交给 PPM 文件或 PPM 帧流.
frame_index 只用于给动画帧命名(lessonXX_0000.ppm),不参与动画逻辑.
*/
static void output_frame(const lesson_t *lesson, const framebuffer_t *fb, int frame_index, output_mode_t mode)
{
    if (mode == OUTPUT_PPM)
    {
        /* 动画按帧号写序列文件,静态课程直接写单个文件 */
        char path[256];
        if (lesson->animated)
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
}

/*
渲染进度条:输出到 stderr,不占用 stdout.
stdout 只留给 PPM 数据,进度提示全部走 stderr.
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
- 静态课程(animated=0):只渲染第 0 帧;
- 动画课程:从第 0 帧渲染到第 ANIMATION_FRAMES-1 帧,
  每帧调用一次 render(fb, frame_index, ANIMATION_FRAMES),再把结果交给 output_frame.
*/
static void run_lesson(const lesson_t *lesson, output_mode_t mode)
{
    /* PPM 输出目录统一放在 out/ 下,已被 .gitignore 忽略 */
    if (mode == OUTPUT_PPM)
        mkdir("out", 0777);

    if (!lesson->animated)
    {
        framebuffer_t fb;
        fb_clear(&fb);
        if (lesson->render(&fb, 0, 1) == 0)
            output_frame(lesson, &fb, 0, mode);
        return;
    }

    /* 动画循环帧数来自全局 ANIMATION_FRAMES,所有动画课程共用同一个速度 */
    int total_frames = ANIMATION_FRAMES;

    if (mode == OUTPUT_PPM_STREAM)
        fprintf(stderr, "渲染 %s:共 %d 帧,正在写入标准输出...\n", lesson->id, total_frames);

    for (int frame_index = 0; frame_index < total_frames; frame_index++)
    {
        framebuffer_t fb;
        fb_clear(&fb);
        if (lesson->render(&fb, frame_index, total_frames) != 0)
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
    /* 无参数:提示用法 */
    if (argc < 2)
    {
        fprintf(stderr, "用法: ray_tracing <lesson> ppm|stream\n");
        fprintf(stderr, "交互式使用请运行 ./ray.sh\n");
        return 1;
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
        printf("%d\n", lesson->animated ? ANIMATION_FRAMES : 0);
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
    {
        fprintf(stderr, "用法: ray_tracing <lesson> ppm|stream\n");
        return 1;
    }

    return 0;
}
