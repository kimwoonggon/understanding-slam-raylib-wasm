#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import math
import os
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


WORLD_WIDTH = 120
WORLD_HEIGHT = 80
CELL_SIZE = 8
IMAGE_WIDTH = WORLD_WIDTH * CELL_SIZE
IMAGE_HEIGHT = WORLD_HEIGHT * CELL_SIZE
MOTION_SPEED = 0.5
FNV_OFFSET = 0xCBF29CE484222325
FNV_PRIME = 0x100000001B3

MAP_OBSTACLE = (80, 80, 80)
LASER = (255, 0, 0)
HIT_AND_ROBOT = (0, 255, 0)


@dataclass
class FrameStat:
    frame: int
    hash_hex: str
    changed: int
    pose: tuple[float, float, float]


@dataclass
class CompareSummary:
    failures: list[str]
    hash_mismatch_count: int
    changed_diff_max: int
    changed_diff_mean: float


def set_pixel(buf: bytearray, x: int, y: int, color: tuple[int, int, int]) -> None:
    if x < 0 or x >= IMAGE_WIDTH or y < 0 or y >= IMAGE_HEIGHT:
        return
    idx = (y * IMAGE_WIDTH + x) * 3
    buf[idx] = color[0]
    buf[idx + 1] = color[1]
    buf[idx + 2] = color[2]


def draw_rect(buf: bytearray, x: int, y: int, width: int, height: int, color: tuple[int, int, int]) -> None:
    for yy in range(height):
        for xx in range(width):
            set_pixel(buf, x + xx, y + yy, color)


def draw_line(buf: bytearray, x0: int, y0: int, x1: int, y1: int, color: tuple[int, int, int]) -> None:
    dx = abs(x1 - x0)
    dy = abs(y1 - y0)
    x_step = 1 if x0 < x1 else -1
    y_step = 1 if y0 < y1 else -1
    err = dx - dy

    while True:
        set_pixel(buf, x0, y0, color)
        if x0 == x1 and y0 == y1:
            break
        err_twice = 2 * err
        if err_twice > -dy:
            err -= dy
            x0 += x_step
        if err_twice < dx:
            err += dx
            y0 += y_step


def draw_circle(buf: bytearray, cx: int, cy: int, radius: int, color: tuple[int, int, int]) -> None:
    radius_sq = radius * radius
    for dy in range(-radius, radius + 1):
        for dx in range(-radius, radius + 1):
            if (dx * dx) + (dy * dy) <= radius_sq:
                set_pixel(buf, cx + dx, cy + dy, color)


def fnv1a64(buf: bytearray) -> str:
    h = FNV_OFFSET
    for b in buf:
        h ^= b
        h = (h * FNV_PRIME) & 0xFFFFFFFFFFFFFFFF
    return f"{h:016x}"


def changed_pixels(prev: bytearray, cur: bytearray) -> int:
    changed = 0
    for i in range(0, min(len(prev), len(cur)), 3):
        if prev[i] != cur[i] or prev[i + 1] != cur[i + 1] or prev[i + 2] != cur[i + 2]:
            changed += 1
    return changed


def load_sequence(path: Path) -> list[str]:
    sequence: list[str] = []
    for line in path.read_text(encoding="utf-8").splitlines():
        token = line.strip().upper()
        if token.startswith("#"):
            continue
        sequence.append(token)
    if not sequence:
        raise RuntimeError(f"input sequence is empty: {path}")
    return sequence


def run_cpp_trace(exe: Path, inputs: Path, world_grid: Path) -> list[FrameStat]:
    cmd = [str(exe), "--inputs", str(inputs)]
    cmd += ["--world-grid", str(world_grid)]

    proc = subprocess.run(cmd, capture_output=True, text=True, check=False)
    if proc.returncode != 0:
        raise RuntimeError(f"cpp trace failed:\nSTDOUT:\n{proc.stdout}\nSTDERR:\n{proc.stderr}")

    frames: list[FrameStat] = []
    for raw_line in proc.stdout.splitlines():
        line = raw_line.strip()
        if not line or not line.startswith("{"):
            continue
        obj = json.loads(line)
        frames.append(
            FrameStat(
                frame=int(obj["frame"]),
                hash_hex=str(obj["hash"]),
                changed=int(obj["changed"]),
                pose=(float(obj["pose"][0]), float(obj["pose"][1]), float(obj["pose"][2])),
            )
        )
    if not frames:
        raise RuntimeError("cpp trace produced no frame data")
    return frames


def run_wasm_trace(node_js: Path, inputs: Path, world_grid: Path) -> list[FrameStat]:
    cmd = ["node", str(node_js), "--inputs", str(inputs), "--world-grid", str(world_grid)]
    proc = subprocess.run(cmd, capture_output=True, text=True, check=False)
    if proc.returncode != 0:
        raise RuntimeError(f"wasm trace failed:\nSTDOUT:\n{proc.stdout}\nSTDERR:\n{proc.stderr}")

    frames: list[FrameStat] = []
    for raw_line in proc.stdout.splitlines():
        line = raw_line.strip()
        if not line or not line.startswith("{"):
            continue
        obj = json.loads(line)
        frames.append(
            FrameStat(
                frame=int(obj["frame"]),
                hash_hex=str(obj["hash"]),
                changed=int(obj["changed"]),
                pose=(float(obj["pose"][0]), float(obj["pose"][1]), float(obj["pose"][2])),
            )
        )
    if not frames:
        raise RuntimeError("wasm trace produced no frame data")
    return frames


class KeyState:
    def __init__(self, pressed: set[int]) -> None:
        self._pressed = pressed

    def __getitem__(self, key: int) -> bool:
        return key in self._pressed


def render_ref2_frame(
    occupancy_map,
    pose,
    scan,
    occupied_value: int,
) -> bytearray:
    frame = bytearray(IMAGE_WIDTH * IMAGE_HEIGHT * 3)

    for y in range(WORLD_HEIGHT):
        for x in range(WORLD_WIDTH):
            if occupancy_map.value_at(x, y) == occupied_value:
                draw_rect(frame, x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE, MAP_OBSTACLE)

    origin_x = int(pose.x * CELL_SIZE)
    origin_y = int(pose.y * CELL_SIZE)
    hits: list[tuple[int, int]] = []
    for sample in scan:
        absolute_angle = pose.theta + sample.relative_angle
        end_x = int((pose.x + math.cos(absolute_angle) * sample.distance) * CELL_SIZE)
        end_y = int((pose.y + math.sin(absolute_angle) * sample.distance) * CELL_SIZE)
        draw_line(frame, origin_x, origin_y, end_x, end_y, LASER)
        if sample.hit:
            hits.append((end_x, end_y))

    for hx, hy in hits:
        draw_circle(frame, hx, hy, 2, HIT_AND_ROBOT)

    draw_rect(frame, int(pose.x * CELL_SIZE) - 3, int(pose.y * CELL_SIZE) - 3, 6, 6, HIT_AND_ROBOT)
    return frame


def load_world_grid(path: Path) -> list[str]:
    rows = [line.strip() for line in path.read_text(encoding="utf-8").splitlines() if line.strip()]
    if len(rows) != WORLD_HEIGHT:
        raise RuntimeError(f"world-grid height mismatch: expected {WORLD_HEIGHT}, got {len(rows)}")
    for y, row in enumerate(rows):
        if len(row) != WORLD_WIDTH:
            raise RuntimeError(f"world-grid width mismatch at y={y}: expected {WORLD_WIDTH}, got {len(row)}")
        for ch in row:
            if ch not in ".#01":
                raise RuntimeError(f"world-grid contains unsupported char: {ch!r}")
    return rows


def run_ref2_trace(ref2_root: Path, sequence: list[str], world_grid: Path) -> list[FrameStat]:
    os.environ.setdefault("PYGAME_HIDE_SUPPORT_PROMPT", "1")
    os.environ.setdefault("SDL_VIDEODRIVER", "dummy")
    os.environ.setdefault("SDL_AUDIODRIVER", "dummy")

    sys.path.insert(0, str(ref2_root / "src"))

    import pygame  # type: ignore
    from slam_understanding.core import OCCUPIED, OccupancyGridMap, RobotPose, SimulatedLidar, WorldGrid
    from slam_understanding.movement import handle_motion

    pygame.init()
    try:
        world = WorldGrid(width=WORLD_WIDTH, height=WORLD_HEIGHT)
        for y, row in enumerate(load_world_grid(world_grid)):
            for x, ch in enumerate(row):
                if ch == "#" or ch == "1":
                    world.set_obstacle(x, y)
        occupancy_map = OccupancyGridMap(width=WORLD_WIDTH, height=WORLD_HEIGHT)
        lidar = SimulatedLidar(max_range=30.0, beam_count=72, step_size=1.0)
        pose = RobotPose(x=10.0, y=10.0, theta=0.0)

        previous = bytearray(IMAGE_WIDTH * IMAGE_HEIGHT * 3)
        frames: list[FrameStat] = []

        for frame_idx, token in enumerate(sequence):
            pressed: set[int] = set()
            if "W" in token:
                pressed.add(pygame.K_w)
                pressed.add(pygame.K_UP)
            if "S" in token:
                pressed.add(pygame.K_s)
                pressed.add(pygame.K_DOWN)
            if "A" in token:
                pressed.add(pygame.K_a)
                pressed.add(pygame.K_LEFT)
            if "D" in token:
                pressed.add(pygame.K_d)
                pressed.add(pygame.K_RIGHT)

            updated_pose = handle_motion(keys=KeyState(pressed), pose=pose, speed=MOTION_SPEED)
            if updated_pose != pose and world.is_obstacle(int(updated_pose.x), int(updated_pose.y)):
                updated_pose = pose
            pose = updated_pose

            scan = lidar.scan(world=world, pose=pose)
            occupancy_map.integrate_scan(pose=pose, scan=scan)

            frame = render_ref2_frame(occupancy_map, pose, scan, OCCUPIED)
            frames.append(
                FrameStat(
                    frame=frame_idx,
                    hash_hex=fnv1a64(frame),
                    changed=changed_pixels(previous, frame),
                    pose=(float(pose.x), float(pose.y), float(pose.theta)),
                )
            )
            previous = frame

        return frames
    finally:
        pygame.quit()


def compare_traces(
    cpp_frames: list[FrameStat],
    ref2_frames: list[FrameStat],
    pose_tol: float,
    max_changed_diff: int,
    strict_hash: bool,
) -> CompareSummary:
    if len(cpp_frames) != len(ref2_frames):
        return CompareSummary(
            failures=[f"frame-count mismatch: left={len(cpp_frames)} right={len(ref2_frames)}"],
            hash_mismatch_count=0,
            changed_diff_max=0,
            changed_diff_mean=0.0,
        )

    failures: list[str] = []
    hash_mismatch_count = 0
    changed_diffs: list[int] = []
    for cpp, ref2 in zip(cpp_frames, ref2_frames):
        if cpp.frame != ref2.frame:
            failures.append(f"frame index mismatch at position {cpp.frame}: raylib={cpp.frame} ref2={ref2.frame}")
            continue

        if cpp.hash_hex != ref2.hash_hex:
            hash_mismatch_count += 1
            if strict_hash:
                failures.append(
                    f"frame {cpp.frame}: hash mismatch raylib={cpp.hash_hex} ref2={ref2.hash_hex}"
                )

        changed_diff = abs(cpp.changed - ref2.changed)
        changed_diffs.append(changed_diff)
        if changed_diff > max_changed_diff:
            failures.append(
                f"frame {cpp.frame}: changed-pixels diff={changed_diff} exceeds max={max_changed_diff} "
                f"(raylib={cpp.changed}, ref2={ref2.changed})"
            )

        dx = abs(cpp.pose[0] - ref2.pose[0])
        dy = abs(cpp.pose[1] - ref2.pose[1])
        dt = abs(cpp.pose[2] - ref2.pose[2])
        if dx > pose_tol or dy > pose_tol or dt > pose_tol:
            failures.append(
                f"frame {cpp.frame}: pose mismatch raylib={cpp.pose} ref2={ref2.pose} tol={pose_tol}"
            )
    max_diff = max(changed_diffs) if changed_diffs else 0
    mean_diff = (sum(changed_diffs) / len(changed_diffs)) if changed_diffs else 0.0
    return CompareSummary(
        failures=failures,
        hash_mismatch_count=hash_mismatch_count,
        changed_diff_max=max_diff,
        changed_diff_mean=mean_diff,
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Differential E2E compare: ref2 pygame vs raylib C++")
    parser.add_argument(
        "--cpp-trace-exe",
        default="build-release/slam-diff-trace",
        help="path to compiled raylib trace executable",
    )
    parser.add_argument(
        "--ref2-root",
        default="../understanding-slam-raylib-wasm-ref2",
        help="path to ref2 pygame project root",
    )
    parser.add_argument(
        "--inputs",
        default="tests/data/e2e_wasd_sequence.txt",
        help="WASD input sequence file",
    )
    parser.add_argument(
        "--world-grid",
        default="tests/data/world_grid_120x80.txt",
        help="shared world-grid file (120x80) used by all engines for deterministic comparison",
    )
    parser.add_argument(
        "--pose-tol",
        type=float,
        default=1e-6,
        help="absolute tolerance for pose value comparison",
    )
    parser.add_argument(
        "--max-changed-diff",
        type=int,
        default=40,
        help="max allowed per-frame absolute difference in changed-pixel count",
    )
    parser.add_argument(
        "--strict-hash",
        action="store_true",
        help="fail on any frame hash mismatch (off by default)",
    )
    parser.add_argument(
        "--wasm-trace-js",
        default="",
        help="optional wasm/node trace js (from DifferentialTrace compiled with em++)",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    cpp_exe = Path(args.cpp_trace_exe).resolve()
    ref2_root = Path(args.ref2_root).resolve()
    inputs = Path(args.inputs).resolve()
    world_grid = Path(args.world_grid).resolve()
    wasm_trace_js = Path(args.wasm_trace_js).resolve() if args.wasm_trace_js else None

    if not cpp_exe.exists():
        print(f"error: cpp trace executable not found: {cpp_exe}", file=sys.stderr)
        return 2
    if not ref2_root.exists():
        print(f"error: ref2 root not found: {ref2_root}", file=sys.stderr)
        return 2
    if not inputs.exists():
        print(f"error: input sequence not found: {inputs}", file=sys.stderr)
        return 2
    if not world_grid.exists():
        print(f"error: world-grid not found: {world_grid}", file=sys.stderr)
        return 2
    if wasm_trace_js is not None and not wasm_trace_js.exists():
        print(f"error: wasm trace js not found: {wasm_trace_js}", file=sys.stderr)
        return 2

    sequence = load_sequence(inputs)
    cpp_frames = run_cpp_trace(exe=cpp_exe, inputs=inputs, world_grid=world_grid)
    ref2_frames = run_ref2_trace(ref2_root=ref2_root, sequence=sequence, world_grid=world_grid)
    raylib_vs_ref2 = compare_traces(
        cpp_frames=cpp_frames,
        ref2_frames=ref2_frames,
        pose_tol=args.pose_tol,
        max_changed_diff=args.max_changed_diff,
        strict_hash=args.strict_hash,
    )

    def print_pair(name: str, left_frames: list[FrameStat], right_frames: list[FrameStat], summary: CompareSummary) -> None:
        print(f"[{name}] frames={len(sequence)}")
        print(
            f"[{name}] left_first_hash={left_frames[0].hash_hex} left_last_hash={left_frames[-1].hash_hex} "
            f"right_first_hash={right_frames[0].hash_hex} right_last_hash={right_frames[-1].hash_hex}"
        )
        print(
            f"[{name}] hash_mismatch_frames={summary.hash_mismatch_count}/{len(sequence)} strict_hash={args.strict_hash}"
        )
        print(
            f"[{name}] changed_diff_max={summary.changed_diff_max} "
            f"changed_diff_mean={summary.changed_diff_mean:.3f} allowed_max={args.max_changed_diff}"
        )

    print_pair("raylib-vs-ref2", cpp_frames, ref2_frames, raylib_vs_ref2)

    failures: list[str] = []
    failures.extend(f"[raylib-vs-ref2] {line}" for line in raylib_vs_ref2.failures)

    if wasm_trace_js is not None:
        wasm_frames = run_wasm_trace(node_js=wasm_trace_js, inputs=inputs, world_grid=world_grid)
        wasm_vs_ref2 = compare_traces(
            cpp_frames=wasm_frames,
            ref2_frames=ref2_frames,
            pose_tol=args.pose_tol,
            max_changed_diff=args.max_changed_diff,
            strict_hash=args.strict_hash,
        )
        wasm_vs_raylib = compare_traces(
            cpp_frames=wasm_frames,
            ref2_frames=cpp_frames,
            pose_tol=args.pose_tol,
            max_changed_diff=args.max_changed_diff,
            strict_hash=args.strict_hash,
        )
        print_pair("wasm-vs-ref2", wasm_frames, ref2_frames, wasm_vs_ref2)
        print_pair("wasm-vs-raylib", wasm_frames, cpp_frames, wasm_vs_raylib)
        failures.extend(f"[wasm-vs-ref2] {line}" for line in wasm_vs_ref2.failures)
        failures.extend(f"[wasm-vs-raylib] {line}" for line in wasm_vs_raylib.failures)

    if failures:
        print("status=FAIL")
        for line in failures[:80]:
            print(line)
        extra = len(failures) - 80
        if extra > 0:
            print(f"... and {extra} more mismatches")
        return 1

    print("status=PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
