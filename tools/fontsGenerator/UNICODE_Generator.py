from PIL import Image, ImageDraw, ImageFont

FONT_PATH = "font.ttf"          # 像素字体
FONT_SIZE = 8                   # 字号
OUTPUT_FILE = "UNICODE_DEFAULT_8x8"

def unicode_bmp_chars():
    """生成 Unicode 所有码位 0x0000->0xFFFF,代理区填 None"""
    chars = []
    for cp in range(0xFFFF + 1):
        if 0xD800 <= cp <= 0xDFFF:
            chars.append(None)
            continue
        try:
            ch = chr(cp)
        except ValueError:
            ch = None
        chars.append(ch)
    return chars

def get_char_width(char, font):
    if char is None:
        return 0
    return int(font.getlength(char) + 0.5) 

def render_char(char, font):
    """
    渲染单个字符为 8x8 纵向取模点阵，高位在下。
    返回 8 字节数组，每字节 bit7=最下像素, bit0=最上像素。
    """
    if char is None:
        return bytearray(8)

    img = Image.new("1", (8, 8), 0)
    draw = ImageDraw.Draw(img)
    draw.text((0, 0), char, font=font, fill=1)

    data = bytearray(8)
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
    print(f"码位总数：{total}")

    with open(OUTPUT_FILE, "wb") as f:
        for ch in chars:
            width = get_char_width(ch, font)
            f.write(bytes([width]))          # 宽度 1 字节
            f.write(render_char(ch, font))   # 点阵 8 字节
        print(f"已写入 {total * 9} 字节 -> {OUTPUT_FILE}")
        print("文件结构：每字符 9 字节 [宽度(1B) + 点阵(8B)]，按码点顺序排列")

if __name__ == "__main__":
    main()