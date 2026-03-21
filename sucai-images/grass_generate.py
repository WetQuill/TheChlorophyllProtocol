import numpy as np
from PIL import Image, ImageFilter


def generate_camouflage_grass(size=64):
    # 定义草地迷彩调色板 (R, G, B)
    palette = [
        (34, 80, 34),  # 深森林绿 (基准)
        (60, 100, 40),  # 橄榄绿 (色块1)
        (100, 140, 60),  # 苔藓绿 (色块2)
        (45, 65, 30),  # 暗部阴影 (色块3)
    ]

    # 1. 初始化基准层
    img_data = np.full((size, size, 3), palette[0], dtype=np.uint8)

    def get_noise_mask(size, blur_radius, threshold):
        # 生成随机噪声并转换为图像以应用 PIL 的模糊滤镜
        noise = np.random.rand(size, size) * 255
        noise_img = Image.fromarray(noise.astype(np.uint8))
        # 核心：通过模糊产生迷彩特有的“圆润色块”
        blurred = noise_img.filter(ImageFilter.GaussianBlur(radius=blur_radius))
        # 通过阈值将其转为布尔遮罩
        mask = np.array(blurred) > threshold
        return mask

    # 2. 叠加迷彩层
    # 橄榄绿色块
    mask1 = get_noise_mask(size, blur_radius=4, threshold=130)
    img_data[mask1] = palette[1]

    # 苔藓绿色块 (亮色)
    mask2 = get_noise_mask(size, blur_radius=3, threshold=160)
    img_data[mask2] = palette[2]

    # 暗部阴影
    mask3 = get_noise_mask(size, blur_radius=5, threshold=180)
    img_data[mask3] = palette[3]

    # 3. 添加草地细节纹理 (杂色)
    texture = (np.random.rand(size, size, 1) - 0.5) * 30
    img_data = np.clip(img_data.astype(np.float32) + texture, 0, 255).astype(np.uint8)

    return Image.fromarray(img_data)


# 执行生成
if __name__ == "__main__":
    size = 64
    grass_tex = generate_camouflage_grass(size)

    # 保存原始 64x64 图像
    grass_tex.save("grass_camouflage_64x64.png")

    # 为了方便预览，生成一个放大版 (最近邻插值保持像素感)
    preview = grass_tex.resize((256, 256), Image.NEAREST)
    preview.show()  # 直接打开预览
    print(f"成功生成迷彩图案并保存为 grass_camouflage_64x64.png")
