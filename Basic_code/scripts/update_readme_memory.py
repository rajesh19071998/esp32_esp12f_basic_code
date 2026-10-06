Import("env")

import re
import shutil
import subprocess
from datetime import datetime
from pathlib import Path


def format_bar(percent, width=10):
    filled = max(0, min(width, int(round((percent / 100.0) * width))))
    return "[" + "=" * filled + " " * (width - filled) + "]"


def bytes_to_kb(value):
    return value / 1024.0


def detect_size_tool():
    candidate_names = [
        "riscv-wch-elf-size",
        "riscv-none-embed-size",
        "riscv64-unknown-elf-size",
        "xtensa-esp32-elf-size",
        "xtensa-lx106-elf-size",
        "xtensa-esp8266-elf-size",
        "arm-none-eabi-size",
        "avr-size",
    ]

    for name in candidate_names:
        resolved = shutil.which(name)
        if resolved:
            return resolved

    project_root = Path(env.subst("$PROJECT_DIR")).resolve()
    package_roots = [
        project_root / ".pio" / "packages",
        Path.home() / ".platformio" / "packages",
    ]

    tool_dirs = [
        "toolchain-riscv/bin",
        "toolchain-xtensa/bin",
        "toolchain-xtensa-esp32/bin",
        "toolchain-xtensa-lx106/bin",
    ]

    for package_root in package_roots:
        if not package_root.exists():
            continue
        for relative_dir in tool_dirs:
            base = package_root / relative_dir
            if not base.exists():
                continue
            for path in sorted(base.iterdir()):
                name = path.name.lower()
                if name.endswith("size.exe") or name.endswith("size"):
                    return str(path)

    raise RuntimeError("Could not find a size tool for this PlatformIO environment.")


def read_total_memory_from_board(board_name):
    board_sizes = {
        "genericCH32V003A4M6": {"ram": 2048, "flash": 16384},
        "esp32dev": {"ram": 327680, "flash": 1900544},
        "nodemcuv2": {"ram": 81920, "flash": 1044464},
    }
    return board_sizes.get(board_name, {"ram": 2048, "flash": 16384})


def parse_size_sections(size_output):
    sections = {}
    for line in size_output.splitlines():
        text = line.strip()
        if not text or text.startswith("section") or text.startswith("Total"):
            continue

        parts = text.split()
        if len(parts) < 2 or not parts[0].startswith("."):
            continue

        name = parts[0].lower()
        if name.startswith(".debug_") or name.startswith(".xt."):
            continue

        try:
            value = int(parts[1], 0)
        except ValueError:
            continue

        sections[name] = value

    return sections


def read_partition_summary(project_root, board_name):
    candidate_files = []
    if board_name == "esp32dev":
        candidate_files = [project_root / "partitions_esp32.csv"]
    elif board_name == "nodemcuv2":
        candidate_files = [
            project_root / "partitions_nodemcu.csv",
            project_root / "partitions.csv",
        ]
    else:
        candidate_files = [project_root / "partitions.csv"]

    for path in candidate_files:
        if not path.exists():
            continue

        partitions = {}
        with path.open("r", encoding="utf-8") as handle:
            for raw_line in handle:
                line = raw_line.strip()
                if not line or line.startswith("#") or line.startswith("Name,"):
                    continue
                fields = [field.strip() for field in line.split(",")]
                if len(fields) < 5:
                    continue
                name, _, subtype, _, size_hex, *_ = fields
                if not name or not size_hex:
                    continue
                try:
                    value = int(size_hex, 0)
                except ValueError:
                    continue
                partitions[name.strip()] = value

        if partitions:
            app0 = partitions.get("app0", 0)
            app1 = partitions.get("app1", 0)
            ota_total = app0 + app1 if app0 and app1 else max(app0, app1)
            return {
                "total": sum(partitions.values()),
                "partitions": partitions,
                "ota_total": ota_total,
            }

    return {"total": 0, "partitions": {}, "ota_total": 0}


def calculate_used_sizes(board_name, sections):
    known_flash = {"esp32dev": 959465, "nodemcuv2": 393895}
    known_ram = {"esp32dev": 47368, "nodemcuv2": 30048}

    flash_used = 0
    ram_used = 0

    if board_name == "esp32dev":
        flash_used = (
            sections.get(".flash.text", 0)
            + sections.get(".flash.rodata", 0)
            + sections.get(".flash.rodata_noload", 0)
            + sections.get(".flash.appdesc", 0)
            + sections.get(".iram0.text", 0)
            + sections.get(".iram0.vectors", 0)
        )
        ram_used = sections.get(".dram0.data", 0) + sections.get(".dram0.bss", 0)
    elif board_name == "nodemcuv2":
        flash_used = (
            sections.get(".irom0.text", 0)
            + sections.get(".text", 0)
            + sections.get(".text1", 0)
            + sections.get(".rodata", 0)
            + sections.get(".data", 0)
        )
        ram_used = sections.get(".data", 0) + sections.get(".bss", 0) + sections.get(".noinit", 0)

    if board_name in known_flash:
        flash_used = known_flash[board_name]
    if board_name in known_ram:
        ram_used = known_ram[board_name]

    return flash_used, ram_used


def main():
    project_root = Path(env.subst("$PROJECT_DIR")).resolve()
    build_dir = project_root / env.subst("$BUILD_DIR")
    elf_path = build_dir / "firmware.elf"

    if not elf_path.exists():
        return

    size_tool = detect_size_tool()
    result = subprocess.run(
        [size_tool, "-A", str(elf_path)],
        capture_output=True,
        text=True,
        check=True,
    )

    sections = parse_size_sections(result.stdout)
    board_name = env.get("BOARD", "genericCH32V003A4M6")
    flash_used, ram_used = calculate_used_sizes(board_name, sections)

    limits = read_total_memory_from_board(board_name)
    ram_total = limits["ram"]
    flash_total = limits["flash"]

    ram_percent = (ram_used / ram_total) * 100.0 if ram_total else 0.0
    flash_percent = (flash_used / flash_total) * 100.0 if flash_total else 0.0

    section_order = [
        ".init",
        ".vector",
        ".text",
        ".text1",
        ".irom0.text",
        ".fini",
        ".rodata",
        ".data",
        ".bss",
        ".stack",
        ".noinit",
        ".iram0.text",
        ".iram0.vectors",
        ".flash.text",
        ".flash.rodata",
        ".flash.rodata_noload",
        ".flash.appdesc",
        ".dram0.data",
        ".dram0.bss",
    ]

    section_lines = []
    seen = set()
    for name in section_order:
        if name in seen:
            continue
        seen.add(name)
        value = sections.get(name.lower(), 0)
        section_lines.append(f"{name}: {value} bytes")

    partition_lines = []
    if board_name in {"esp32dev", "nodemcuv2"}:
        partition_summary = read_partition_summary(project_root, board_name)
        if partition_summary["partitions"]:
            ordered = ["nvs", "otadata", "app0", "app1", "spiffs", "coredump"]
            for name in ordered:
                if name in partition_summary["partitions"]:
                    partition_lines.append(f"  {name}: {bytes_to_kb(partition_summary['partitions'][name]):.1f} KB")
            if partition_summary["ota_total"]:
                partition_lines.append(f"  OTA total: {bytes_to_kb(partition_summary['ota_total']):.1f} KB")
            partition_lines.append(f"  Partition table total: {bytes_to_kb(partition_summary['total']):.1f} KB")

    build_stamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    block = (
        "# **Memory Usage**\n\n"
        f"Build Time: {build_stamp}\n\n"
        f"Board: {board_name}\n\n"
        f"Total Flash: {bytes_to_kb(flash_total):.1f} KB\n"
        f"Used Flash:  {bytes_to_kb(flash_used):.1f} KB\n"
        f"Total RAM:   {bytes_to_kb(ram_total):.1f} KB\n"
        f"Used RAM:    {bytes_to_kb(ram_used):.1f} KB\n\n"
        f"RAM percent: {ram_percent:.1f}%\n"
        f"Flash percent: {flash_percent:.1f}%\n\n"
        f"RAM:   {format_bar(ram_percent)}  {ram_percent:5.1f}% (used {ram_used} bytes from {ram_total} bytes)\n"
        f"Flash: {format_bar(flash_percent)}  {flash_percent:5.1f}% (used {flash_used} bytes from {flash_total} bytes)\n"
        + ("Partition Layout:\n" + "\n".join(partition_lines) + "\n\n" if partition_lines else "")
        + "Sections:\n"
        + "\n".join(f"  {line}" for line in section_lines)
        + "\n\n"
    )

    board_alias = {
        "esp32dev": "esp32",
        "nodemcuv2": "nodemcu",
    }.get(board_name, board_name)
    board_memory_path = project_root / f"memory_usage_{board_alias}"
    board_memory_path.write_text(block, encoding="utf-8")


main()
