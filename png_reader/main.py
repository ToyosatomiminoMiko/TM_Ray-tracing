from PIL import Image
from pathlib import Path
from typing import List

def load_texture(file_path: Path) -> List[List[int]]:
    """
    加载16x16 PNG材质文件并转换为三维列表结构

    Args:
        file_path: PNG文件路径
        
    Returns:
        三维列表,结构为[行][列][R,G,B,A]
        
    Raises:
        ValueError: 如果图像尺寸不是16x16
    """
    try:
        img = Image.open(file_path).convert("RGBA")
    except Exception as e:
        raise ValueError(f"无法加载图像: {e}")
    
    if img.size != (16, 16):
        raise ValueError(f"图像尺寸应为16x16,但实际为{img.size}")
    
    pixels = img.load()
    texture_data = []
    for y in range(16):
        for x in range(16):
            texture_data.append(list(pixels[x, y]))
    return texture_data

if __name__ == "__main__":
    # 使用示例
    try:
        # 加载材质文件
        texture = load_texture(Path("./glass.png"))
        print("textures loaded")
        print(texture)
        
    except FileNotFoundError:
        print("error: file not found textures")
    except ValueError as e:
        print(f"error: {e}")
