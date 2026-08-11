from PIL import Image, ImageDraw, ImageFont

FONT_PATH = "font.ttf"          # 像素字体
FONT_SIZE = 8                  # 字号
OUTPUT_FILE = "GB2312_DEFAULT_8x8"

def gb2312_chars():
    """生成 GB2312 所有可解码字符, 顺序为区位码升序, 未定义位置记 None"""
    chars = []
    for zone in range(0xA1, 0xF8):      # 区号 01–87 内码高字节 0xA1–0xF7
        for bit in range(0xA1, 0xFF):    # 位号 01–94 内码低字节 0xA1–0xFE
            try:
                ch = bytes([zone, bit]).decode('gb2312')
                chars.append(ch)
            except UnicodeDecodeError:
                chars.append(None)
    return chars

def render_char(char, font):
    """
    渲染单个字符为 8x8 纵向取模点阵，高位在下。
    返回 8 字节数组, 每字节 bit7=最下像素, bit0=最上像素。
    """
    if char is None:
        return bytearray(8)

    # 创建 8x8 的二值图像, 黑底白字
    img = Image.new("1", (8, 8), 0)
    draw = ImageDraw.Draw(img)
    draw.text((0, 0), char, font=font, fill=1)

    data = bytearray(8)  # 8 列，初始全零
    for y in range(8):
        for x in range(8):
            if img.getpixel((x, y)):
                data[x] |= 1 << y   # 高位在下：y 越大位越高
    return data

def main():
    try:
        font = ImageFont.truetype(FONT_PATH, FONT_SIZE)
    except OSError:
        print(f"错误: 无法加载字体 {FONT_PATH}, 请确认文件存在且为 TrueType 字体")
        return

    chars = gb2312_chars()
    total = len(chars)
    undefined = chars.count(None)
    print(f"码位总数:{total}, 其中未定义: {undefined}, 有效字符: {total - undefined}")

    with open(OUTPUT_FILE, "wb") as f:
        for ch in chars:
            bitmap = render_char(ch, font)
            f.write(bitmap)
        print(f"已写入 {total * 8} 字节 -> {OUTPUT_FILE}")

if __name__ == "__main__":
    main()
