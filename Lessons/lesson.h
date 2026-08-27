#ifndef LESSON_H_
#define LESSON_H_

#include <stdint.h>

#include "framebuffer.h"

/*
课程接口:
每节课只负责把一帧画进 framebuffer_t(内存),
输出(终端动画 / PPM 文件)由 main 统一处理。
*/
typedef struct
{
    const char *id;        /* 命令行里使用的 id,例如 lesson03 */
    const char *title;     /* 课程标题 */
    int frames;            /* 0 = 静态单帧;>0 = 动画总帧数 */
    uint32_t frame_delay_us; /* 动画帧间隔(微秒) */
    int (*render)(framebuffer_t *fb, int frame); /* 渲染第 frame 帧,返回非 0 表示提前结束 */
} lesson_t;

/* 按 id 查找课程,找不到返回 NULL */
const lesson_t *lesson_find(const char *id);

/* 默认课程(最新的扩展课) */
const lesson_t *lesson_default(void);

/* 打印所有课程 */
void lesson_list(void);

#endif /* LESSON_H_ */
