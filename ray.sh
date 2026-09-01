#!/usr/bin/env bash
# 用户交互层:选课菜单/管道给 ffmpeg 生成 mp4/gif.
# 输出方式只提供 ppm / mp4 / gif 三种,不提供终端播放.
# C 程序只负责渲染,所有"问用户要什么"的事情都在这里.
set -euo pipefail

cd "$(dirname "$0")"

# 帧率只属于编码端:C 端动画只按帧数渲染,不关心播放帧率.
# MP4 默认 60fps;GIF 格式限制锁 25fps,编码时用 fps=25 从 60fps 帧流里取帧,
# 循环时长 = 帧数 / 60,动画画面本身不受帧率影响.
MP4_FPS=60
GIF_FPS=25

if [[ -x ./ray_tracing ]]; then
    RAY_BIN=./ray_tracing
elif [[ -x ./build/ray_tracing ]]; then
    RAY_BIN=./build/ray_tracing
else
    echo "找不到 ray_tracing,请先构建: cmake -S . -B build && cmake --build build" >&2
    exit 1
fi

usage() {
    cat <<'EOF'
用法:
  ./ray.sh                  交互式选课菜单
  ./ray.sh list             列出课程
  ./ray.sh <lesson> mp4     渲染并通过管道编码 out/<lesson>.mp4
  ./ray.sh <lesson> gif     渲染并通过管道编码 out/<lesson>.gif
  ./ray.sh <lesson> ppm     保存 PPM 到 out/(静态课单帧,动画课帧序列)
EOF
}

require_ffmpeg() {
    if ! command -v ffmpeg >/dev/null 2>&1; then
        echo "未找到 ffmpeg,请先安装(如: sudo apt install ffmpeg)" >&2
        exit 1
    fi
}

# 把 C 渲染出的 PPM 帧流直接管道给 ffmpeg,不落中间文件.
encode() {
    local id=$1 fmt=$2 meta frames stream_fps gif_fps

    if ! meta=$("$RAY_BIN" meta "$id" 2>/dev/null); then
        echo "找不到课程 $id" >&2
        exit 1
    fi
    frames=$meta

    if (( frames <= 0 )); then
        echo "提示:$id 是静态课程,只会生成单帧画面." >&2
        stream_fps=1
        gif_fps=1
    else
        stream_fps=$MP4_FPS
        gif_fps=$GIF_FPS
    fi

    mkdir -p out
    case "$fmt" in
        mp4)
            echo "正在生成 out/$id.mp4 ..."
            "$RAY_BIN" "$id" stream |
                ffmpeg -y -loglevel warning \
                    -f image2pipe -framerate "$stream_fps" -vcodec ppm -i - \
                    -c:v libx264 -preset medium -crf 18 \
                    -pix_fmt yuv420p -movflags +faststart \
                    "out/$id.mp4"
            ;;
        gif)
            echo "正在生成 out/$id.gif ..."
            "$RAY_BIN" "$id" stream |
                ffmpeg -y -loglevel warning \
                    -f image2pipe -framerate "$stream_fps" -vcodec ppm -i - \
                    -vf "fps=$gif_fps,split[s0][s1];[s0]palettegen=stats_mode=full[p];[s1][p]paletteuse=dither=bayer:bayer_scale=5" \
                    -loop 0 \
                    "out/$id.gif"
            ;;
    esac
    echo "完成: out/$id.$fmt"
}

# 交互式选课:菜单 -> 输出方式 -> 运行.
interactive() {
    local lines=() ids=() labels=() id title total

    # 第一行是 C 程序打印的 "可用课程:" 表头,跳过它
    mapfile -t lines < <("$RAY_BIN" list | tail -n +2)
    for line in "${lines[@]}"; do
        read -r id title <<<"$line"
        ids+=("$id")
        labels+=("$id  $title")
    done
    total=${#ids[@]}

    while true; do
        echo "可用课程:"
        PS3="请选择课程编号(输入 $((total + 1)) 退出): "
        select choice in "${labels[@]}" "退出"; do
            if [[ -z $REPLY ]]; then
                echo "已取消"
                exit 0
            fi
            if [[ $REPLY -ge 1 && $REPLY -le $total ]]; then
                break
            elif [[ $REPLY -eq $((total + 1)) ]]; then
                exit 0
            fi
            echo "无效选择,请重试"
        done

        id=${ids[$((REPLY - 1))]}
        echo "已选择:$id"
        PS3="请选择输出方式(输入 4 返回选课): "
        select fmt in "生成 mp4" "生成 gif" "保存 ppm" "返回选课"; do
            if [[ -z $REPLY ]]; then
                echo "已取消"
                exit 0
            fi
            case $REPLY in
                1) require_ffmpeg; encode "$id" mp4; return 0 ;;
                2) require_ffmpeg; encode "$id" gif; return 0 ;;
                3) "$RAY_BIN" "$id" ppm; return 0 ;;
                4) break ;;
                *) echo "无效选择,请重试" ;;
            esac
        done
    done
}

case "${1:-}" in
    "") interactive ;;
    -h | --help | help) usage ;;
    list) "$RAY_BIN" list ;;
    *)
        id=$1
        fmt=${2:-}
        case "$fmt" in
            mp4 | gif) require_ffmpeg; encode "$id" "$fmt" ;;
            ppm) "$RAY_BIN" "$id" ppm ;;
            *) usage; exit 1 ;;
        esac
        ;;
esac
