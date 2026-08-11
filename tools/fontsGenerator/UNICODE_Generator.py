from PIL import Image, ImageDraw, ImageFont

FONT_PATH = "font.ttf"          # 像素字体
FONT_SIZE = 8                   # 字号
OUTPUT_FILE = "UNICODE_DEFAULT_8x8"

def unicode_bmp_chars():
    """生成 Unicode 所有码位0x0000-0xFFFF, 代理区(0xD800-0xDFFF)填 None。"""
    chars = []
    for cp in range(0xFFFF + 1):   # 0 ~ 0xFFFF
        if 0xD800 <= cp <= 0xDFFF:
            chars.append(None)
            continue
        try:
            ch = chr(cp)
        except ValueError:      # 窄 Python 下无法生成代理对字符
            ch = None
        chars.append(ch)
    return chars

def render_char(char, font):
    """
    渲染单个字符为 8x8 纵向取模点阵，高位在下。
    返回 8 字节数组, 每字节 bit7=最下像素, bit0=最上像素。
    """
    if char is None:
        return bytearray(8)

    # 创建 8x8 的二值图像，黑底白字
    img = Image.new("1", (8, 8), 0)
    draw = ImageDraw.Draw(img)
    draw.text((0, 0), char, font=font, fill=1)

    data = bytearray(8)  # 8 列
    for y in range(8):
        for x in range(8):
            if img.getpixel((x, y)):
                data[x] |= 1 << y   # 高位在下
    return data

def main():
    try:
        font = ImageFont.truetype(FONT_PATH, FONT_SIZE)
    except OSError:
        print(f"错误：无法加载字体 {FONT_PATH}")
        return

    chars = unicode_bmp_chars()
    total = len(chars)
    undefined = chars.count(None)
    print(f"码位总数：{total}，其中未定义/代理区：{undefined}")

    with open(OUTPUT_FILE, "wb") as f:
        for ch in chars:
            bitmap = render_char(ch, font)
            f.write(bitmap)
        print(f"已写入 {total * 8} 字节 -> {OUTPUT_FILE}")

if __name__ == "__main__":
    main()