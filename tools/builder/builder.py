import shutil
from pathlib import Path


def main():
    script_dir = Path(__file__).resolve().parent
    apps_dir = script_dir.parent.parent / "apps"
    target_dir = script_dir / "APPs"

    print(f"脚本目录: {script_dir}")
    print(f"搜索目录: {apps_dir}")
    print(f"目标目录: {target_dir}")

    project_files = [p for p in apps_dir.rglob("Project") if p.is_file()]

    if not project_files:
        print("未找到任何 Project 文件")
        return

    target_dir.mkdir(parents=True, exist_ok=True)

    copied = 0
    for src in project_files:
        try:
            rel = src.relative_to(apps_dir)
        except ValueError:
            continue

        if len(rel.parts) < 2:
            continue

        top_folder = rel.parts[0]
        dest_file = target_dir / top_folder

        try:
            shutil.copy2(src, dest_file)
            print(f"已复制: {src} -> {dest_file}")
            copied += 1
        except Exception as e:
            print(f"复制失败: {src} - {e}")

    print(f"完成, 共复制 {copied} 个 Project 文件")


if __name__ == "__main__":
    main()