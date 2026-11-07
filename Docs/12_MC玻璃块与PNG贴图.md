# MC 玻璃块与 PNG 贴图

## 本课目标

Lesson12 把一张 16x16 的 Minecraft 玻璃贴图贴到三维方块上,用 alpha 混合让中间的透明像素露出背景.三个新东西:

1. **PNG 解码**:PNG 自带 alpha,解码后统一成 RGBA8;
2. **方块面 UV**:光线命中 6 个面之一时,把命中点换算成该面的 (u, v);
3. **透明混合**:同一条光线先后穿过的多个面,按距离从远到近叠加.

本课**只做展示**:光线碰到玻璃后方向不变,不计算折射,反射或菲涅尔(留给后续支线,见文末"扩展点").所以玻璃像"带花纹的透明薄片",不是物理正确的玻璃.

## 为什么 PPM 不够:需要 alpha

Lesson11 的 PPM 只有 RGB,没有 alpha;`texture_load_ppm` 加载时把 alpha 一律填 255(完全不透明).玻璃必须区分"看得见"和"看得穿"的像素:

- alpha = 255:完全不透明(玻璃边框);
- alpha = 0:完全透明(玻璃中间);
- 0 < alpha < 255:半透明,按比例混合.

于是 Texture 改成统一 RGBA8:每个像素 4 字节,行优先;PPM 的 alpha 填 255,PNG 保留文件里的 alpha.采样返回的 RGB 与 alpha 分开存放,alpha **不预乘**,混合时再乘.

## PNG 文件格式

**签名**:固定 8 字节 `89 50 4E 47 0D 0A 1A 0A`(中间是 ASCII 的 "PNG").`png.c` 先 `memcmp`,不对直接失败.

**chunk**:每个 chunk 是四段:

```text
长度(4 字节,大端) | 类型(4 字节 ASCII) | 数据(长度字节) | CRC(4 字节,大端)
```

长度只算数据;CRC 覆盖"类型 + 数据",用 zlib 的 `crc32()` 校验;解析时先检查长度越界,再校验 CRC.

本课用到的 chunk:

| chunk | 作用 | png.c 里的处理 |
| --- | --- | --- |
| `IHDR` | 宽,高,位深,颜色类型,压缩/滤波/隔行方法 | 必须第一个,长度 13 |
| `PLTE` | 调色板,最多 256 组 RGB | 颜色类型 3 必须提供 |
| `tRNS` | 调色板 alpha,或灰度/RGB 颜色键 | 决定部分像素的 alpha |
| `IDAT` | 压缩后的像素数据,可多块 | 先拼接,再一次性解压 |
| `IEND` | 文件结束 | 缺失判失败 |

IHDR 的 13 字节依次是宽(4),高(4),位深(1),颜色类型(1),压缩(1),滤波(1),隔行(1).

**只支持的教学子集**:

- 位深 8;16 位深需要样本缩放,本课不做;
- 非隔行(interlace = 0);Adam7 要分 7 趟解码,本课不做;
- 颜色类型 0/2/3/4/6:灰度,RGB,调色板,灰度+alpha,RGBA;
- tRNS:调色板 alpha,以及灰度/RGB 颜色键透明.

不支持的组合会往 stderr 打印原因并返回 -1,不会返回"颜色错了"的图;失败时 `PngImage` 清零.本课素材 `png_reader/glass.png` 是 16x16,位深 8,颜色类型 6(RGBA),非隔行,正好在支持范围内.

**解压**:所有 IDAT 拼起来是一个 zlib 流(2 字节头 + deflate + 4 字节 Adler-32),用 zlib 的 `uncompress()` 一次解压.解压后按扫描线排列,每行 = 1 字节滤波器编号 + width \* bpp 字节样本,总大小 = height \* (1 + width * bpp).bpp 是文件内每像素字节数:灰度 1,RGB 3,调色板 1,灰度+alpha 2,RGBA 4.解压后必须检查长度恰好等于上式.

**反滤波**:编码时先用"左边/上边/左上"预测当前字节,只把残差交给 zlib,解码要把预测值加回去.设当前字节 x,左 a,上 b,左上 c(第一列/第一行按 0),所有加法模 256:

| 编号 | 名称 | 还原公式 |
| --- | --- | --- |
| 0 | None | $x$ |
| 1 | Sub | $x + a$ |
| 2 | Up | $x + b$ |
| 3 | Average | $x + \lfloor (a+b)/2 \rfloor$ |
| 4 | Paeth | $x + \operatorname{Paeth}(a,b,c)$ |

Paeth 先算 $p = a + b - c$,取 p 到 a/b/c 距离最小者,相等时按 a -> b -> c:

$$
\operatorname{Paeth}(a,b,c) = \begin{cases} a, & |p-a| \le |p-b| \text{ 且 } |p-a| \le |p-c| \\ b, & |p-b| \le |p-c| \\ c, & \text{否则} \end{cases}
$$

Sub/Average/Paeth 里的左邻要往前跳 bpp 字节(跳过整个像素);滤波器编号逐行可以不同.

**统一转 RGBA8**:灰度 r = g = b = 样本;RGB 取三个样本;调色板用样本查 PLTE,alpha 取 tRNS 对应项(没有则 255);灰度+alpha 取两个样本;RGBA 取四个样本.灰度/RGB 的颜色键命中时 alpha 置 0,其余 alpha 默认 255.输出行优先 RGBA8,上层 `texture.c` 不用关心源格式.

## 贴图采样:最近邻 + alpha

方块面的 UV 是连续浮点数,采样用 `texture_sample_rgba_nearest`:取最近像素,返回 0..1 的 RGB,并把 alpha 写进 out_alpha.

- u 循环包裹(`u - floor(u)`),v 夹紧到 [0,1];
- 返回的 alpha 已归一化到 0..1(PNG 里存的是 0..255);
- 返回的 RGB 未乘 alpha;
- 加载失败返回品红且 alpha=1,一眼看出贴图没找到.

`glass.png` 的 alpha 只有 0 和 255 两档(这份素材没有半透明像素):外圈边框不透明,中间透明,角附近还有少量不透明细节.混合公式对 0..255 任意 alpha 都成立.

## 方块面求交

方块是轴对齐立方体:中心 `block_center`,半边长 `block_half`,边长 1.8,包围盒 lo/hi = center 减/加 half.

6 个面都当"无限平面"求交.对第 f 个面(法线沿 axis,side = +1/-1),平面上该轴坐标固定:

$$
t = \frac{\text{plane} - O_{\text{axis}}}{D_{\text{axis}}}, \qquad \text{plane} = C_{\text{axis}} + \text{side} \cdot \text{half}
$$

四个检查:

1. `|D_axis| < 1e-6`:平行,跳过;
2. `t <= 1e-4`:在相机背后,跳过;
3. 另外两个轴上的坐标必须在 [lo, hi] 内(1e-5 容差);
4. 通过则记录 t,单位法线,该面的 (u, v).

第 3 步把无限平面裁成方块的一个面,这就是"6 个面当平面求交 + 面内矩形检查".一条射线穿过凸立方体最多命中 2 个面,但函数仍按最多 6 个命中来写,数组留 6 个位置.

### 为什么按 t 排序

透明混合要求从远到近,所以命中必须按 t 从大到小处理.求交时先按 t 升序做插入排序(命中少,插入排序最简单),渲染时倒序遍历,就是从远到近.

### 命中点映射到面的 (u, v)

每个面的 UV 都从"站在面外看这个面"的视角定义,贴图不会镜像:u 向右,v 向下.记 size = 2 * half:

| 面 | u | v |
| --- | --- | --- |
| +X | $(hi.z - P.z)/size$ | $(hi.y - P.y)/size$ |
| -X | $(P.z - lo.z)/size$ | $(hi.y - P.y)/size$ |
| +Y | $(P.x - lo.x)/size$ | $(P.z - lo.z)/size$ |
| -Y | $(P.x - lo.x)/size$ | $(hi.z - P.z)/size$ |
| +Z | $(P.x - lo.x)/size$ | $(hi.y - P.y)/size$ |
| -Z | $(hi.x - P.x)/size$ | $(hi.y - P.y)/size$ |

例如正对相机的 +Z 面:u 沿 +X,v 沿 -Y,所以 u = P.x - lo.x,v = hi.y - P.y.其他面"向右/向下"的轴不同,原则不变.

## 相机与光线

相机参数写在 `lesson12.c` 里:眼睛 (2.2, 1.5, 1.8),看向方块中心 (0, 0, -3.4),垂直视场角 36 度,画面 1600x900.先构造基向量(做法同 Lesson08):

$$
f = \operatorname{normalize}(\text{target} - \text{eye}), \quad r = \operatorname{normalize}(f \times (0,1,0)), \quad u = r \times f
$$

再对像素中心算 NDC:$\text{ndc}_x = 2(i+0.5)/W - 1$,$\text{ndc}_y = 1 - 2(j+0.5)/H$(y 取反,因为图像坐标向下).视场角展开:

$$
\text{half}_h = \tan(\text{fov}/2), \quad \text{half}_w = \text{half}_h \cdot W/H
$$

$$
\text{dir} = \operatorname{normalize}(f + r \cdot \text{ndc}_x \cdot \text{half}_w + u \cdot \text{ndc}_y \cdot \text{half}_h)
$$

光线起点是眼睛,dir 已归一化,所以 t 就是距离.

## Alpha 混合

每个像素先算背景,再从远到近把玻璃面"盖"上去.每个面的贡献按下面公式混合(每个通道独立):

$$
C_{\text{out}} = C_{\text{src}} \cdot \alpha + C_{\text{dst}} \cdot (1 - \alpha)
$$

$C_{\text{src}}$ 是当前面被简单光照后的颜色,$C_{\text{dst}}$ 是之前累积的颜色,$\alpha$ 是该像素的 alpha(0..1).三个关键点:

1. **从远到近**:先背景,再最远的面,最后最近的面;顺序反了,远处会错误盖住近处.
2. **alpha = 1 完全遮挡,alpha = 0 完全不遮挡**:alpha=0 时 $C_{\text{out}} = C_{\text{dst}}$,透明像素不挡背景;中间值按比例混合.
3. **alpha 不预乘**:采样得到的 RGB 是贴图原始颜色,混合时再乘 alpha;两种约定混用会让透明边缘发黑或过亮.

`lesson12.c` 的核心循环(命中已按 t 升序,所以倒序遍历):

```c
for (int k = hit_count - 1; k >= 0; k--)
{
    float alpha;
    vec3 rgb = sample_glass(hits[k].u, hits[k].v, &alpha);
    vec3 lit = vec3_scale(rgb, face_brightness(hits[k].normal));
    color = vec3_add(vec3_scale(lit, alpha), vec3_scale(color, 1.0f - alpha));
}
```

## 背景棋盘格与面的明暗

背景是 z = -7.5 的无限大平面,画成 1 米一格的棋盘:用 floor(P.x) 与 floor(P.y) 的奇偶决定两种蓝灰色,再按 P.y 加一点从上到下变暗的渐变.

为什么用棋盘格?透明区域需要参照物:玻璃中间透明处能看到棋盘格,说明那些像素 alpha=0;不透明边框则完全盖住背景,显示浅青色.背景若是纯色,很难一眼判断哪里透,哪里不透.

玻璃面只做最简光照:

$$
\text{brightness} = 0.55 + 0.45 \cdot \max(N \cdot L, 0)
$$

N 是面法线,L 是归一化平行光方向.目的只是让正面/侧面/顶面亮度不同,方便看出立体感,不是真实玻璃光照:没有高光,没有环境反射,也没有折射.

## 运行

```sh
./ray.sh lesson12 ppm                 # 在仓库根目录运行,输出 out/lesson12.ppm
./build/ray_tracing lesson12 ppm      # 直接调用二进制,同样输出 out/lesson12.ppm
```

lesson12 在课程注册表里 `animated = 0`,是静态课程:只渲染一帧,`./build/ray_tracing meta lesson12` 输出 0(动画课程才连续渲染多帧交给 mp4/gif).

素材路径是 `png_reader/glass.png`;lesson12 依次尝试 `png_reader/glass.png` 和 `../png_reader/glass.png`. `ray.sh` 会先切到仓库根目录,所以第一种可用;在 `build/` 里直接运行二进制时第二种兜底.找不到素材会打印警告,回退到"边缘不透明,中间透明"的程序化配色.

构建需要 zlib:PNG 的 IDAT 用 zlib 解压,CMake 里用 `find_package(ZLIB REQUIRED)` 查找并链接 `ZLIB::ZLIB`;系统一般自带,报找不到时安装 zlib 开发包.

## 扩展点

本课只做展示,真正的玻璃效果留作后续支线,这里只列方向,不展开实现:

- 折射:用 Snell 定律在进入/离开玻璃时改变光线方向;
- 反射:玻璃表面按菲涅尔比例同时产生反射光线;
- 菲涅尔:视线越斜反射越强,正对时几乎全透.

这些都属于"光线方向会变"的问题,和本课"方向不变,只混合颜色"是两个层次.
