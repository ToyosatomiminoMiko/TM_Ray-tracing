# TM_Ray-tracing

光线追踪教学课程(C 语言).
课程按学习顺序组织,所有渲染统一先写入内存帧缓冲,
再从内存输出为终端动画或 PPM 文件.

## 目录结构

```text
main.c               课程入口与输出模式控制
Core/math            vec3/mat4:纯数学函数,注释带公式
Core/render          内存帧缓冲 + 终端/PPM 输出
Core/raytrace        光线/球/三角形/平面求交
Lessons/             每节课一个目录,Lesson01 到 Lesson09
Docs/                理论和方法文档
```

详细说明见 [Docs/README.md](Docs/README.md).

## 建议阅读顺序

1. [main.c](main.c):统一渲染循环,理解"内存帧缓冲 -> 输出"的数据流;
2. [Lessons/registry.c](Lessons/registry.c):课程注册表,理解动画时长/帧率怎么声明;
3. `Lessons/Lesson01_ColorGradient/`:第一课,最简单的"每像素一条光线";
4. `Core/math/`、`Core/raytrace/`:用到哪个看哪个,注释里都带公式.

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
./build/ray_tracing              # 默认运行最后一课(抗锯齿)
./build/ray_tracing list         # 列出所有课程
./build/ray_tracing lesson05     # 终端动画(静态课程显示一帧)
./build/ray_tracing lesson05 ppm # 渲染到内存后输出 out/lesson05.ppm
```

动画课程加 `ppm` 参数会按帧输出 `out/lessonXX_0000.ppm` 序列,
可用 ImageMagick 合成 GIF:

```sh
magick -delay 5 out/lesson08_*.ppm out/lesson08.gif
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
