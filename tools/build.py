import shutil
from pathlib import Path


def main():
    script_dir = Path(__file__).resolve().parent          # 脚本所在目录
    parent_dir = script_dir.parent                        # 父级目录（搜索根目录）
    target_dir = script_dir / "APPs"

    # 需要排除的顶层文件夹名
    exclude_folders = {".STD_Template", ".HAL_Template"}

    print(f"脚本目录   : {script_dir}")
    print(f"搜索根目录 : {parent_dir}")
    print(f"目标目录   : {target_dir}")
    print(f"排除顶层文件夹: {exclude_folders}")

    project_files = [p for p in parent_dir.rglob("Project") if p.is_file()]

    if not project_files:
        print("未找到任何 Project 文件。")
        return

    target_dir.mkdir(parents=True, exist_ok=True)

    copied = 0
    for src in project_files:
        if script_dir in src.parents:
            print(f"跳过脚本目录内的文件: {src}")
            continue

        try:
            rel = src.relative_to(parent_dir)
        except ValueError:
            print(f"无法获取相对路径: {src}")
            continue

        if len(rel.parts) < 2:
            print(f"跳过根目录下的 Project: {src}")
            continue

        top_folder = rel.parts[0]

        if top_folder in exclude_folders:
            print(f"已排除: {src}")
            continue

        dest_file = target_dir / top_folder

        try:
            shutil.copy2(src, dest_file)
            print(f"已复制: {src}  ->  {dest_file}")
            copied += 1
        except Exception as e:
            print(f"复制失败: {src}  -  {e}")

    print(f"\n完成，共复制 {copied} 个 Project 文件。")


if __name__ == "__main__":
    main()