# TM_Ray-tracing

光线追踪简明过程

## build

```sh
mkdir build
cd build
cmake ..
cmake --build .
```

## run

```sh
build/matrix
```

## ppm

```c
// 打开文件
FILE *fp = fopen("output.ppm", "w");
// 写入头 P3 表示纯文本 RGB
fprintf(fp, "P3\n%d %d\n255\n", W, H);
// 写入像素值
fprintf(fp, "%d %d %d\n", r, g, b);
// 相当于绘制点
set_point_color(i, j, r << 16 | g << 8 | b);
// 关闭文件
fclose(fp);
```
