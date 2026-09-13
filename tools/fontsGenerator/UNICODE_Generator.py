from PIL import Image, ImageDraw, ImageFont
import os

BASE_DIR = os.path.dirname(os.path.abspath(__file__))

FONT_PATH = os.path.join(BASE_DIR, "spleen-5x8.bdf")
FONT_SIZE = 8
OUTPUT_FILE = os.path.join(BASE_DIR, "UNICODE-SpleenMono-Regular-6x8")

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

def get_char_width_ttf(char, font):
    if char is None:
        return 0
    return int(font.getlength(char) + 0.5)

def render_char_ttf(char, font):
    """渲染单个字符为 8x8 纵向取模点阵，高位在下。"""
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

def parse_bdf(filepath):

    font_data = {}
    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
        lines = f.readlines()

    i = 0
    while i < len(lines):
        line = lines[i].strip()
        if line.startswith('STARTCHAR'):
            encoding = None
            bbx = None
            dwidth = None
            bitmap_lines = []
            i += 1
            while i < len(lines) and not lines[i].strip().startswith('ENDCHAR'):
                l = lines[i].strip()
                if l.startswith('ENCODING'):
                    parts = l.split()
                    if len(parts) >= 2:
                        try:
                            encoding = int(parts[1], 0) 
                        except ValueError:
                            pass
                elif l.startswith('DWIDTH'):
                    parts = l.split()
                    if len(parts) >= 2:
                        try:
                            dwidth = int(parts[1], 0)
                        except ValueError:
                            pass
                elif l.startswith('BBX'):
                    parts = l.split()
                    if len(parts) >= 5:
                        try:
                            width = int(parts[1], 0)
                            height = int(parts[2], 0)
                            xoff = int(parts[3], 0)
                            yoff = int(parts[4], 0)
                            bbx = (width, height, xoff, yoff)
                        except ValueError:
                            pass
                elif l == 'BITMAP':
                    i += 1
                    while i < len(lines) and not lines[i].strip().startswith('ENDCHAR'):
                        bitmap_lines.append(lines[i].strip())
                        i += 1
                    break
                i += 1

            if encoding is not None and bbx is not None and bitmap_lines:
                width, height, xoff, yoff = bbx
                canvas = [[0] * 8 for _ in range(8)]  # canvas[y][x]

                for row_idx, hex_line in enumerate(bitmap_lines):
                    if row_idx >= 8:
                        break
                    if not hex_line:
                        continue
                    try:
                        data_bytes = bytes.fromhex(hex_line)
                    except ValueError:
                        continue
                    bytes_count = (width + 7) // 8
                    for byte_idx in range(min(len(data_bytes), bytes_count)):
                        byte = data_bytes[byte_idx]
                        for bit in range(8):
                            x = byte_idx * 8 + (7 - bit)  # MSB 在最左
                            if x >= width or x >= 8:
                                continue
                            if byte & (1 << bit):
                                if row_idx < 8:
                                    canvas[row_idx][x] = 1

                # 转换为原代码的 data 格式: data[x] 的 bit y 对应 (x, y)
                data = bytearray(8)
                for x in range(8):
                    for y in range(8):
                        if canvas[y][x]:
                            data[x] |= 1 << y

                # 宽度优先使用 DWIDTH, 否则用 BBX 宽度
                w = dwidth if dwidth is not None else width
                w = min(max(w, 0), 255)
                font_data[encoding] = (w, data)
        i += 1
    return font_data

def main():
    if FONT_PATH.lower().endswith('.bdf'):
        print("使用 BDF 解析模式")
        font_data = parse_bdf(FONT_PATH)
        chars = unicode_bmp_chars()
        total = len(chars)
        print(f"码位总数：{total}")

        with open(OUTPUT_FILE, "wb") as f:
            for cp, ch in enumerate(chars):
                if ch is None:
                    f.write(bytes([0]))
                    f.write(bytearray(8))
                else:
                    if cp in font_data:
                        w, data = font_data[cp]
                        f.write(bytes([w]))
                        f.write(data)
                    else:
                        f.write(bytes([0]))
                        f.write(bytearray(8))
        print(f"已写入 {total * 9} 字节 -> {OUTPUT_FILE}")
        print("文件结构：每字符 9 字节 [宽度(1B) + 点阵(8B)]，按码点顺序排列")

    else:
        print("使用 TTF 渲染模式")
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
                width = get_char_width_ttf(ch, font)
                f.write(bytes([width]))
                f.write(render_char_ttf(ch, font))
        print(f"已写入 {total * 9} 字节 -> {OUTPUT_FILE}")
        print("文件结构：每字符 9 字节 [宽度(1B) + 点阵(8B)]，按码点顺序排列")

if __name__ == "__main__":
    main()