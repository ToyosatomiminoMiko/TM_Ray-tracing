# TM_Ray-tracing

光线追踪教学课程(C 语言).
课程按学习顺序组织,所有渲染统一先写入内存帧缓冲,
再导出为 PPM 文件,或把 PPM 帧流编码为 mp4/gif.

## 目录结构

```text
main.c               课程入口与输出模式控制
ray.sh               shell 入口:交互选课 + ffmpeg 视频编码
Core/math            vec3/mat4:纯数学函数,注释带公式
Core/render          内存帧缓冲 + PPM 输出(output_ansi 保留供终端调试)
Core/raytrace        光线/球/三角形/平面求交 + 贴图
Lessons/             每节课一个目录,Lesson01 到 Lesson11(10/11 为支线)
Docs/                理论和方法文档
```

详细说明见 [Docs/README.md](Docs/README.md).

## 建议阅读顺序

1. [main.c](main.c):统一渲染循环,理解"内存帧缓冲 -> 输出"的数据流;
2. [Lessons/registry.c](Lessons/registry.c):课程注册表,理解动画怎么声明(只标记是否动画,速度由全局设置控制,播放帧率由编码端决定);
3. `Lessons/Lesson01_ColorGradient/`:第一课,最简单的"每像素一条光线";
4. `Core/math/`/`Core/raytrace/`:用到哪个看哪个,注释里都带公式.

遇到看不懂的业务逻辑时,先看对应函数开头的"课程目标"注释,
再顺着函数里的分步注释读,每个计算都标了公式和几何含义.

## 构建

```sh
mkdir build
cd build
cmake ..
cmake --build .
```

生成可执行文件 `build/ray_tracing`.

## 运行

```sh
./ray.sh                         # 交互式选课菜单
./ray.sh list                    # 列出所有课程
./ray.sh lesson08 mp4            # 逐帧管道给 ffmpeg,输出 out/lesson08.mp4
./ray.sh lesson08 gif            # 逐帧管道给 ffmpeg,输出 out/lesson08.gif
./ray.sh lesson05 ppm            # 渲染到内存后输出 out/lesson05.ppm
```

命令行与交互菜单只提供 ppm / mp4 / gif 三种输出;
`output_ansi` 函数保留在 `Core/render/output.c` 中,仅调试用.

动画编码不落中间文件:C 程序把每一帧 PPM 写到标准输出,
`ray.sh` 用管道直接交给 ffmpeg 生成 mp4/gif.
底层二进制也保留了 `stream` 模式,可自行接任意管道:

```sh
./build/ray_tracing lesson08 stream | ffmpeg -f image2pipe -vcodec ppm -i - out/lesson08.mp4
```

## 课程表

| 课程 | 内容 |
| --- | --- |
| lesson01 | 颜色渐变:像素方向映射颜色 |
| lesson02 | 立方体线框:矩阵变换 + Bresenham 画线(动画) |
| lesson03 | 光线追踪立方体:三角形求交(动画) |
| lesson04 | 球体:一元二次方程求交 |
| lesson05 | 光照与阴影:漫反射 |
| lesson06 | 多球 + 镜面高光:Blinn-Phong |
| lesson07 | 平面棋盘格 + 反射递归 |
| lesson08 | 轨道相机 + 透视投影(动画) |
| lesson09 | 超采样抗锯齿(左右对比) |
| lesson10 | 多线程渲染(支线,左右对比单线程/多线程耗时) |
| lesson11 | 贴图/材质(支线):world.ppm 世界地图 -> 旋转地球(动画) |

动画是帧驱动的:所有动画课程共用全局 `ANIMATION_FRAMES`(默认 240 帧,
60fps 下 4 秒一圈),画面由帧号决定;
MP4 默认 60fps/GIF 固定 25fps 只是编码端的播放设置,不影响动画本身.

lesson11 需要仓库根目录的 `world.ppm`(等距圆柱世界地图)作为贴图素材;
运行 `ray.sh` 时脚本会先切到仓库根目录,贴图路径总能找到.
