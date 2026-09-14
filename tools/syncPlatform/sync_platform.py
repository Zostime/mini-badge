"""
将 CubeMX 生成的文件同步到 src/drivers/platform/<platform>/
"""

import shutil
import sys
from pathlib import Path

PLATFORM = "stm32f1"

DIR_MAP = {
    "Drivers/STM32F1xx_HAL_Driver": "hal",
    "Drivers/CMSIS":                "cmsis",
    "Src":                          "board",
}

FILE_MAP = {
    "Inc/stm32f1xx_hal_conf.h":     "stm32f1xx_hal_conf.h",
    "Inc/stm32f1xx_it.h":           "board/stm32f1xx_it.h",
    "Inc/spi.h":                    "board/spi.h",
    "Inc/tim.h":                    "board/tim.h",
    "Inc/adc.h":                    "board/adc.h",
    "Inc/rtc.h":                    "board/rtc.h",
    "Inc/gpio.h":                   "board/gpio.h",
}   

EXCLUDE_FILES = {
    "main.c",
    "system_stm32f1xx.c",
    "usb_device.c",
    "usbd_conf.c",
    "usbd_desc.c",
    "usbd_cdc_if.c",
}

def find_repo_root(start: Path) -> Path:
    """从脚本目录向上找，直到发现 .git 目录"""
    cur = start.resolve()
    for _ in range(10):
        if (cur / ".git").exists():
            return cur
        cur = cur.parent
    raise RuntimeError("找不到 Git 仓库根 (应含 .git/)")


def sync_dir(src: Path, dst: Path, exclude: set = None):
    exclude = exclude or set()
    if not src.is_dir():
        print(f"  [跳过] {src}")
        return
    if dst.exists():
        shutil.rmtree(dst)

    def ignore_fn(dir_, names):
        return {n for n in names if n in exclude}

    shutil.copytree(src, dst, ignore=ignore_fn)
    n = sum(1 for _ in dst.rglob("*") if _.is_file())
    print(f"  [同步] {src.name} -> {dst.relative_to(dst.parents[5])}  ({n} 文件)")


def sync_file(src: Path, dst: Path):
    if not src.is_file():
        print(f"  [跳过] {src}")
        return
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst)
    print(f"  [同步] {src.name} -> {dst.relative_to(dst.parents[5])}")


def main():
    cubemx_root = Path(__file__).parent.resolve()  
    repo_root = find_repo_root(cubemx_root)

    if not list(cubemx_root.glob("*.ioc")):
        print(f"[警告] 在 {cubemx_root} 未找到 .ioc 文件")

    print(f"CubeMX 根:  {cubemx_root}")
    print(f"仓库根:     {repo_root}")

    dst_root = repo_root / "src" / "drivers" / "platform" / PLATFORM
    dst_root.mkdir(parents=True, exist_ok=True)

    for src_rel, dst_name in DIR_MAP.items():
        if dst_name.startswith("../"):
            dst_path = (dst_root / dst_name).resolve()
        else:
            dst_path = dst_root / dst_name

        excl = EXCLUDE_FILES if dst_name == "board" else set()
        sync_dir(cubemx_root / src_rel, dst_path, excl)

    for src_rel, dst_name in FILE_MAP.items():
        sync_file(cubemx_root / src_rel, dst_root / dst_name)

    print("\n同步完成。")


if __name__ == "__main__":
    try:
        main()
        while True:
            input("按 Enter 键退出...")
            break
    except Exception as e:
        print(f"错误: {e}", file=sys.stderr)
        sys.exit(1)