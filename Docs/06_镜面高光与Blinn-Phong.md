# 镜面高光(Blinn-Phong)

对应 Lesson06:多个球 + 镜面高光.
基础的环境光/漫反射/阴影见[第 05 篇](05_光照模型.md).

## 为什么需要高光

漫反射只描述"粗糙表面":颜色随光照角度变化,但没有亮斑.
光滑表面(水面、塑料、金属)会在特定角度反射光源,
形成一块随视线移动的高光.高光项和观察方向有关:

视线方向 \( V \),半程向量:

$$
H = \operatorname{normalize}(L + V)
$$

\( H \) 是"光源方向与视线方向的中间方向",表面法线越接近 \( H \),
反射的光线越能进入眼睛.

## 高光公式

$$
\text{spec} = \max(N \cdot H, 0)^{\text{shininess}}
$$

`shininess` 越大,高光越集中(越"亮片"):

- 小值(如 16):高光范围大,像哑光塑料;
- 大值(如 128):高光又小又亮,像光滑金属.

## 最终颜色

在漫反射结果上叠加高光(高光通常用白色或带一点场景色):

$$
\text{color} = \text{albedo} \cdot \bigl(\text{ambient} + (1 - \text{ambient}) \cdot \text{diff}\bigr) + \text{spec\_color} \cdot \text{spec}
$$

## 整体流程

```text
主光线 -> 找最近物体 -> 命中点 P/法线 N
  -> L = normalize(light - P)
  -> 阴影光线判定
  -> diff = max(N·L, 0)
  -> H = normalize(L + V)
  -> spec = pow(max(N·H, 0), shininess)
  -> 汇总颜色
```
