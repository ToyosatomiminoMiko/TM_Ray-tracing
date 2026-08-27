#ifndef OUTPUT_H_
#define OUTPUT_H_

#include "framebuffer.h"

/*
统一输出层:
课程只负责把画面画进 framebuffer_t(内存),
这里提供两种"从内存读出"的展示方式:
1. output_ansi:终端真彩色动画
2. output_ppm :写入 PPM 文件
*/

/* ANSI 清屏:`\x1b[3J` 清除回滚缓冲区,`\x1b[2J` 清空屏幕,`\x1b[H` 光标回左上角 */
void output_clear_screen(void);

/* 把内存帧缓冲按 ANSI 真彩色块输出到终端 */
void output_ansi(const framebuffer_t *fb);

/*
把内存帧缓冲写入 PPM 文件(P3 纯文本格式):
第一行 P3,第二行 宽 高,第三行 255,之后每行一个像素的 RGB。
成功返回 0,失败返回 -1。
*/
int output_ppm(const framebuffer_t *fb, const char *path);

#endif /* OUTPUT_H_ */
