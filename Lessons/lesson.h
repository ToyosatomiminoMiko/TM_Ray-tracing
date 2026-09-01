#ifndef LESSON_H_
#define LESSON_H_

#include <stdint.h>

#include "framebuffer.h"

/*
课程接口:
每节课只负责把一帧画进 framebuffer_t(内存),
输出(PPM 文件 / PPM 帧流)由 main 统一处理.

动画采用"帧驱动"而不是"时间驱动":
所有动画课程共用同一个全局速度 ANIMATION_FRAMES(一整个循环多少帧),
课程自己只标记"是不是动画",不各自设置速度.
render 收到的是当前帧号 frame_index 和循环总帧数 frame_count.
画面只由"这是第几帧"决定,和秒/帧率完全无关:
- 帧率只存在于 ray.sh 编码端(MP4 60fps / GIF 25fps),同一个循环两种格式都能播;
- 想调快/调慢所有动画,只改 ANIMATION_FRAMES 一处.
*/

/* 全局动画速度:一整个动画循环渲染多少帧.
   默认 240 帧 = MP4 60fps 下 4 秒转一圈,足够看清运动过程. */
#define ANIMATION_FRAMES 240

typedef struct
{
    const char *id;     /* 命令行里使用的 id,例如 lesson03 */
    const char *title;  /* 课程标题 */
    int animated;       /* 1 = 动画课程(速度用全局 ANIMATION_FRAMES);0 = 静态课程,只渲染一帧 */
    int (*render)(framebuffer_t *fb, int frame_index, int frame_count); /* 渲染第 frame_index 帧 */
} lesson_t;

/*
循环动画进度:第 frame_index 帧占整个循环的比例,范围 [0, 1).
例如 240 帧转一圈:第 0 帧 = 0 度,第 239 帧 = 359 度,
下一帧回到 0 度,首尾正好无缝衔接.
*/
static inline float lesson_loop_progress(int frame_index, int frame_count)
{
    return (float)frame_index / (float)frame_count;
}

/* 按 id 查找课程,找不到返回 NULL */
const lesson_t *lesson_find(const char *id);

/* 默认课程(最新的扩展课) */
const lesson_t *lesson_default(void);

/* 打印所有课程 */
void lesson_list(void);

#endif /* LESSON_H_ */
