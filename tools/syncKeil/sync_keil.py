import argparse
import copy
import re
import shutil
import sys
from pathlib import Path
import xml.etree.ElementTree as ET

ET.register_namespace('xsi', 'http://www.w3.org/2001/XMLSchema-instance')

SRC_APP = "cat"
DST_APP = ".template"  # 留空则同步 apps 下所有非源 app
MODE = "replace"  # "replace" 直接替换, "merge" 合并

TAGS_TO_REPLACE = (".//FilePath", ".//FileName", ".//GroupName")


def find_git_root(start: Path) -> Path | None:
    """从 start 开始向上查找包含 .git 的目录"""
    cur = start.resolve()
    for parent in [cur, *cur.parents]:
        if (parent / ".git").exists():
            return parent
    return None


def find_apps_dir(script_dir: Path,
                  cli_apps_dir: str | None,
                  cli_root: str | None) -> Path:
    """定位 apps 目录"""
    # 1. 显式指定 apps 目录
    if cli_apps_dir:
        p = Path(cli_apps_dir).resolve()
        if p.is_dir():
            return p
        raise SystemExit(f"apps 目录不存在: {p}")

    # 2. 显式指定 git 根目录
    if cli_root:
        root = Path(cli_root).resolve()
        apps = root / "apps"
        if apps.is_dir():
            return apps
        raise SystemExit(f"{root} 下没有 apps 目录")

    # 3. 自动找 git 根目录
    git_root = find_git_root(script_dir)
    if git_root:
        apps = git_root / "apps"
        if apps.is_dir():
            return apps
        for child in git_root.iterdir():
            if child.is_dir() and (child / "apps").is_dir():
                return (child / "apps").resolve()
        raise SystemExit(f"Git 根 {git_root} 下未找到 apps 目录，请用 --apps-dir 指定")

    for p in [
        script_dir / "apps",
        script_dir.parent / "apps",
        script_dir.parent.parent / "apps",
        script_dir.parent.parent.parent / "apps",
        Path.cwd() / "apps",
    ]:
        if p.is_dir():
            return p.resolve()

    raise SystemExit("找不到 apps 目录，且未检测到 Git 根目录，请用 --apps-dir 指定")

def replace_app(text: str, src_app: str, dst_app: str) -> str:
    """把路径里的 src_app 片段替换成 dst_app，只在 \\ / ; 或边界处替换"""
    if not text:
        return text
    pattern = re.compile(
        r'(^|[\\/;])' + re.escape(src_app) + r'(?=[\\/;]|$)'
    )
    return pattern.sub(lambda m: m.group(1) + dst_app, text)


def replace_tree_app(elem, src_app: str, dst_app: str) -> None:
    for tag in TAGS_TO_REPLACE:
        for el in elem.findall(tag):
            if el.text:
                el.text = replace_app(el.text, src_app, dst_app)


def find_uvprojx(app_dir: Path, src_rel: Path | None = None) -> Path | None:
    """在 app 目录下查找 .uvprojx，优先与源工程相对路径一致的文件"""
    if src_rel:
        candidate = app_dir / src_rel
        if candidate.is_file():
            return candidate

    files = list(app_dir.rglob("*.uvprojx"))
    if not files:
        return None

    for f in files:
        if f.parent.name.lower() == "project":
            return f
    return files[0]


def merge_include_path(dst_text: str, src_text: str, src_app: str, dst_app: str) -> str:
    src_text = replace_app(src_text or "", src_app, dst_app)
    existing = [p.strip() for p in (dst_text or "").split(";") if p.strip()]
    existing_lower = {p.lower() for p in existing}
    for p in (src_text or "").split(";"):
        p = p.strip()
        if p and p.lower() not in existing_lower:
            existing.append(p)
            existing_lower.add(p.lower())
    return ";".join(existing)


def merge_groups(dst_target, src_groups, src_app: str, dst_app: str) -> None:
    dst_groups = dst_target.find("Groups")
    if dst_groups is None:
        dst_groups = ET.SubElement(dst_target, "Groups")

    dst_by_name = {}
    for g in dst_groups.findall("Group"):
        name_el = g.find("GroupName")
        if name_el is not None and name_el.text:
            dst_by_name[name_el.text] = g

    for src_g in src_groups.findall("Group"):
        name_el = src_g.find("GroupName")
        group_name = name_el.text if name_el is not None and name_el.text else None

        if group_name and group_name in dst_by_name:
            dst_g = dst_by_name[group_name]
            dst_files_el = dst_g.find("Files")
            src_files_el = src_g.find("Files")
            if src_files_el is None:
                continue
            if dst_files_el is None:
                new_files_el = copy.deepcopy(src_files_el)
                replace_tree_app(new_files_el, src_app, dst_app)
                dst_g.append(new_files_el)
                continue

            existing_names = set()
            for f in dst_files_el.findall("File"):
                fn = f.find("FileName")
                if fn is not None and fn.text:
                    existing_names.add(fn.text)

            for src_f in src_files_el.findall("File"):
                fn = src_f.find("FileName")
                if fn is None or not fn.text:
                    continue
                if fn.text in existing_names:
                    continue
                new_f = copy.deepcopy(src_f)
                replace_tree_app(new_f, src_app, dst_app)
                dst_files_el.append(new_f)
                existing_names.add(fn.text)
        else:
            new_g = copy.deepcopy(src_g)
            replace_tree_app(new_g, src_app, dst_app)
            dst_groups.append(new_g)


def sync_one(src_file: Path, dst_file: Path, src_app: str, dst_app: str,
             backup_dir: Path) -> None:
    src_tree = ET.parse(src_file)
    src_root = src_tree.getroot()

    src_include_paths = [el.text or "" for el in src_root.findall(".//IncludePath")]
    src_groups = src_root.find(".//Groups")

    dst_tree = ET.parse(dst_file)
    dst_root = dst_tree.getroot()

    # 1. IncludePath
    dst_include_nodes = dst_root.findall(".//IncludePath")
    if src_include_paths:
        for i, node in enumerate(dst_include_nodes):
            src_text = (
                src_include_paths[i]
                if i < len(src_include_paths)
                else src_include_paths[0]
            )
            if MODE == "merge":
                node.text = merge_include_path(node.text or "", src_text,
                                               src_app, dst_app)
            else:
                node.text = replace_app(src_text, src_app, dst_app)

    # 2. Groups
    if src_groups is not None:
        dst_target = dst_root.find(".//Target")
        if dst_target is None:
            raise RuntimeError(f"{dst_file} 中找不到 <Target>")

        if MODE == "merge":
            merge_groups(dst_target, src_groups, src_app, dst_app)
        else:
            dst_groups = dst_target.find("Groups")
            if dst_groups is None:
                dst_groups = ET.SubElement(dst_target, "Groups")

            for child in list(dst_groups):
                dst_groups.remove(child)

            for child in src_groups:
                dst_groups.append(copy.deepcopy(child))

    # 3. 替换 FilePath / FileName / GroupName 里的源 app 名
    if MODE == "replace":
        for tag in TAGS_TO_REPLACE:
            for el in dst_root.findall(tag):
                if el.text:
                    el.text = replace_app(el.text, src_app, dst_app)

    # 备份到 backups/<dst_app>.uvprojx.bak
    backup_dir.mkdir(parents=True, exist_ok=True)
    backup = backup_dir / f"{dst_app}.uvprojx.bak"
    shutil.copy2(dst_file, backup)

    try:
        ET.indent(dst_tree, space="  ")
    except AttributeError:
        pass

    dst_tree.write(dst_file, encoding="utf-8", xml_declaration=True)
    print(f"-> {dst_file}")

def main() -> None:
    parser = argparse.ArgumentParser(
        description="同步 Keil .uvprojx 的 IncludePath 和 Groups"
    )
    parser.add_argument(
        "dst_apps", nargs="*",
        help="目标应用目录名, 如 clear init ls; 留空则同步 apps 下所有非源应用",
    )
    parser.add_argument("--apps-dir", help="直接指定 apps 目录路径")
    parser.add_argument("--root", help="直接指定 Git 根目录路径")
    parser.add_argument("--src", default=SRC_APP,
                        help=f"源应用目录名，默认 {SRC_APP}")
    parser.add_argument("--dry-run", action="store_true",
                        help="只打印不修改")
    args = parser.parse_args()

    script_dir = Path(__file__).resolve().parent
    backup_dir = script_dir / "backups"

    apps_dir = find_apps_dir(script_dir, args.apps_dir, args.root)
    print(f"[info] 使用 apps 目录: {apps_dir}")

    src_app_dir = apps_dir / args.src
    if not src_app_dir.is_dir():
        raise SystemExit(f"源应用目录不存在: {src_app_dir}")

    src_uvprojx = find_uvprojx(src_app_dir)
    if not src_uvprojx:
        raise SystemExit(f"源应用下找不到 .uvprojx: {src_app_dir}")

    src_rel = src_uvprojx.relative_to(src_app_dir)
    print(f"[info] 源工程: {src_uvprojx}")
    print(f"[info] 模式: {MODE}")

    if args.dst_apps:
        dst_names = args.dst_apps
    elif DST_APP:
        dst_names = [DST_APP]
    else:
        dst_names = [
            p.name for p in apps_dir.iterdir()
            if p.is_dir()
            and p.name != args.src
            and not p.name.startswith(".")
        ]
        dst_names = [
            name for name in dst_names
            if find_uvprojx(apps_dir / name, src_rel)
        ]

    if not dst_names:
        raise SystemExit("没有指定目标应用，且未找到可同步的应用")

    print(f"[info] 目标应用: {', '.join(dst_names)}")

    if args.dry_run:
        for name in dst_names:
            dst_uvprojx = find_uvprojx(apps_dir / name, src_rel)
            if dst_uvprojx:
                print(f"-> {dst_uvprojx}")
        print(f"[dry-run] 未修改任何文件，备份将存放于 {backup_dir}")
        return

    synced = 0
    for name in dst_names:
        dst_app_dir = apps_dir / name
        if not dst_app_dir.is_dir():
            print(f"跳过，目标目录不存在: {dst_app_dir}", file=sys.stderr)
            continue

        dst_uvprojx = find_uvprojx(dst_app_dir, src_rel)
        if not dst_uvprojx:
            print(f"跳过，找不到 .uvprojx: {dst_app_dir}", file=sys.stderr)
            continue

        try:
            sync_one(src_uvprojx, dst_uvprojx, args.src, name, backup_dir)
            synced += 1
        except Exception as e:
            print(f"同步失败 {dst_uvprojx}: {e}", file=sys.stderr)

    if synced:
        print(f"已全部备份至 {backup_dir}")


if __name__ == "__main__":
    main()