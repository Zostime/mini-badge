import shutil
import sys
from pathlib import Path

DEFAULT_TARGET_SUBDIRS = ["APPs/", "../../sdcard/bin/", "E:/bin/"]


def resolve_target_dirs(script_dir: Path) -> list[Path]:
    args = sys.argv[1:]
    if args:
        return [Path(a).expanduser().resolve() for a in args]

    result = []
    for name in DEFAULT_TARGET_SUBDIRS:
        p = Path(name).expanduser()
        if not p.is_absolute():
            p = script_dir / p
        result.append(p.resolve())
    return result


def main():
    script_dir = Path(__file__).resolve().parent
    apps_dir = script_dir.parent.parent / "apps"
    target_dirs = resolve_target_dirs(script_dir)

    print(f"脚本目录: {script_dir}")
    print(f"搜索目录: {apps_dir}")
    for i, t in enumerate(target_dirs, 1):
        print(f"目标目录[{i}]: {t}")

    project_files = [p for p in apps_dir.rglob("Project") if p.is_file()]
    if not project_files:
        print("未找到任何 Project 文件")
        return

    # 预先创建所有目标目录
    usable_targets = []
    for t in target_dirs:
        try:
            t.mkdir(parents=True, exist_ok=True)
            usable_targets.append(t)
        except Exception as e:
            print(f"跳过不可用目标目录: {t} - {e}")

    if not usable_targets:
        print("没有任何可用的目标目录")
        return

    total_copied = 0
    for src in project_files:
        try:
            rel = src.relative_to(apps_dir)
        except ValueError:
            continue

        if len(rel.parts) < 2:
            continue

        top_folder = rel.parts[0]

        for target_dir in usable_targets:
            dest_file = target_dir / top_folder
            try:
                shutil.copy2(src, dest_file)
                print(f"已复制: {src} -> {dest_file}")
                total_copied += 1
            except Exception as e:
                print(f"复制失败: {src} -> {dest_file} - {e}")

    print(f"完成, 共复制 {total_copied} 个 Project 文件")
    print(f"共处理 {len(project_files)} 个源文件 × {len(usable_targets)} 个目标目录")


if __name__ == "__main__":
    main()
    while True:
        input("按 Enter 键退出...")
        break