import shutil
from pathlib import Path

file = "Bootloader_api.h"

def main():
    script_dir = Path(__file__).resolve().parent
    source_file = script_dir / file

    if not source_file.is_file():
        print(f"错误：源文件不存在 - {source_file}")
        return

    parent_dir = script_dir.parent
    print(f"搜索目录：{parent_dir}")
    print(f"源文件：{source_file}")

    target_files = list(parent_dir.rglob(file))

    replaced = 0
    for target in target_files:
        if target.resolve() == source_file.resolve():
            continue
        try:
            shutil.copy2(source_file, target) 
            print(f"已替换：{target}")
            replaced += 1
        except Exception as e:
            print(f"替换失败：{target} - {e}")

    print(f"\n完成，共替换 {replaced} 个文件。")


if __name__ == "__main__":
    main()