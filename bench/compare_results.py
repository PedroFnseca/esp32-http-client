"""
compare_results.py
==================
Parses the result.txt files from both ESP32 HTTP benchmarks and generates
comparison charts saved to benchmarking/results/.

Expected result.txt line formats
---------------------------------
Per-request (success):
    [REQ   1/100] OK   902 ms   heap: 187468 -> 186048 bytes

Per-request (fail):
    [REQ   1/10] FAIL  729 ms  [HTTP -1: connection refused]

Summary block (after the separator):
    Total requests     : 100
    Success            : 100
    Failed             : 0
    Min     : 865 ms
    Avg     : 879.9 ms
    Max     : 902 ms
    Min     : 376 bytes  (0.4 KB)
    Avg     : 451 bytes  (0.4 KB)
    Max     : 1420 bytes  (1.4 KB)
    Absolute min    : 131796 bytes  (128.7 KB)
    Initial heap    : 187468 bytes  (183.1 KB)
    Final heap      : 186072 bytes  (181.7 KB)
    Estimate        : 0.7%

Usage
-----
    python compare_results.py

Outputs (saved to benchmarking/results/)
-----------------------------------------
    01_latency_per_request.png   – per-request latency line chart
    02_latency_stats.png         – Min / Avg / Max grouped bar chart
    03_heap_used_per_request.png – heap consumed per request line chart
    04_heap_stats.png            – heap Min / Avg / Max grouped bar chart
    05_system_heap.png           – absolute minimum free heap bar chart
    06_fragmentation.png         – heap fragmentation bar chart
    07_success_rate.png          – success-rate donut charts
    08_dashboard.png             – full summary dashboard
"""

from __future__ import annotations

import io
import os
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional

if sys.stdout.encoding and sys.stdout.encoding.lower() != "utf-8":
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8", errors="replace")

import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import matplotlib.gridspec as gridspec
import numpy as np

SCRIPT_DIR   = Path(__file__).parent
RESULTS_DIR  = SCRIPT_DIR / "results"
RESULTS_DIR.mkdir(parents=True, exist_ok=True)

BENCH_A_RESULT = SCRIPT_DIR / "bench_esp32_http_client"    / "result.txt"
BENCH_B_RESULT = SCRIPT_DIR / "bench_httpclient_arduinojson" / "result.txt"

LABEL_A = "esp32-http-client"
LABEL_B = "HTTPClient + ArduinoJson"

COLOR_A   = "#4FC3F7"
COLOR_B   = "#FF8A65" 
COLOR_BG  = "#0D1117"
COLOR_FG  = "#E6EDF3"
COLOR_GRID= "#21262D"
COLOR_AX  = "#161B22"
FONT      = "DejaVu Sans"

plt.rcParams.update({
    "figure.facecolor":   COLOR_BG,
    "axes.facecolor":     COLOR_AX,
    "axes.edgecolor":     COLOR_GRID,
    "axes.labelcolor":    COLOR_FG,
    "axes.titlecolor":    COLOR_FG,
    "axes.grid":          True,
    "grid.color":         COLOR_GRID,
    "grid.linewidth":     0.6,
    "xtick.color":        COLOR_FG,
    "ytick.color":        COLOR_FG,
    "text.color":         COLOR_FG,
    "legend.facecolor":   COLOR_AX,
    "legend.edgecolor":   COLOR_GRID,
    "legend.labelcolor":  COLOR_FG,
    "font.family":        FONT,
    "font.size":          11,
    "figure.dpi":         150,
})

@dataclass
class BenchResult:
    label: str
    req_numbers:   list[int]   = field(default_factory=list)
    req_times_ms:  list[float] = field(default_factory=list)
    heap_before:   list[int]   = field(default_factory=list)
    heap_after:    list[int]   = field(default_factory=list)
    heap_used:     list[int]   = field(default_factory=list)
    req_ok:        list[bool]  = field(default_factory=list)
    total:         int   = 0
    success:       int   = 0
    failed:        int   = 0
    time_min:      float = 0.0
    time_avg:      float = 0.0
    time_max:      float = 0.0
    heap_used_min: float = 0.0
    heap_used_avg: float = 0.0
    heap_used_max: float = 0.0
    sys_heap_min:  float = 0.0
    heap_initial:  float = 0.0
    heap_final:    float = 0.0
    frag_pct:      float = 0.0

_RE_REQ_OK   = re.compile(
    r'\[REQ\s+(\d+)/\d+\]\s+OK\s+(\d+)\s+ms\s+heap:\s*(\d+)\s*->\s*(\d+)')
_RE_REQ_FAIL = re.compile(
    r'\[REQ\s+(\d+)/\d+\]\s+FAIL\s+(\d+)\s+ms')

_RE_TOTAL    = re.compile(r'Total requests\s*:\s*(\d+)')
_RE_SUCCESS  = re.compile(r'Success\s*:\s*(\d+)')
_RE_FAILED   = re.compile(r'Failed\s*:\s*(\d+)')

_RE_TIME_MIN = re.compile(r'Min\s*:\s*([\d.]+)\s*ms')
_RE_TIME_AVG = re.compile(r'Avg\s*:\s*([\d.]+)\s*ms')
_RE_TIME_MAX = re.compile(r'Max\s*:\s*([\d.]+)\s*ms')

_RE_HEAP_MIN = re.compile(r'Min\s*:\s*(-?[\d.]+)\s*bytes')
_RE_HEAP_AVG = re.compile(r'Avg\s*:\s*(-?[\d.]+)\s*bytes')
_RE_HEAP_MAX = re.compile(r'Max\s*:\s*(-?[\d.]+)\s*bytes')

_RE_SYS_MIN  = re.compile(r'Absolute min\s*:\s*([\d.]+)\s*bytes')
_RE_HEAP_INI = re.compile(r'Initial heap\s*:\s*([\d.]+)\s*bytes')
_RE_HEAP_FIN = re.compile(r'Final heap\s*:\s*([\d.]+)\s*bytes')
_RE_FRAG     = re.compile(r'Estimate\s*:\s*([\d.]+)%')


def parse_result(path: Path, label: str) -> BenchResult:
    r = BenchResult(label=label)

    if not path.exists():
        print(f"[WARN] File not found: {path}")
        return r

    text = path.read_text(encoding="utf-8", errors="replace")
    lines = text.splitlines()

    for line in lines:
        m = _RE_REQ_OK.search(line)
        if m:
            r.req_numbers.append(int(m.group(1)))
            r.req_times_ms.append(float(m.group(2)))
            hb = int(m.group(3)); ha = int(m.group(4))
            r.heap_before.append(hb)
            r.heap_after.append(ha)
            r.heap_used.append(hb - ha)
            r.req_ok.append(True)
            continue
        m = _RE_REQ_FAIL.search(line)
        if m:
            r.req_numbers.append(int(m.group(1)))
            r.req_times_ms.append(float(m.group(2)))
            r.heap_before.append(0)
            r.heap_after.append(0)
            r.heap_used.append(0)
            r.req_ok.append(False)

    in_time_section      = False
    in_heap_used_section = False

    time_mins, time_avgs, time_maxs = [], [], []
    heap_mins, heap_avgs, heap_maxs = [], [], []

    for line in lines:
        if "[TIME PER REQUEST]" in line:
            in_time_section = True; in_heap_used_section = False; continue
        if "[HEAP USED PER REQUEST" in line:
            in_heap_used_section = True; in_time_section = False; continue
        if "[SYSTEM MINIMUM HEAP" in line or "[HEAP FRAGMENTATION" in line:
            in_time_section = False; in_heap_used_section = False

        if m := _RE_TOTAL.search(line):   r.total   = int(m.group(1))
        if m := _RE_SUCCESS.search(line): r.success = int(m.group(1))
        if m := _RE_FAILED.search(line):  r.failed  = int(m.group(1))
        if m := _RE_SYS_MIN.search(line): r.sys_heap_min  = float(m.group(1))
        if m := _RE_HEAP_INI.search(line):r.heap_initial  = float(m.group(1))
        if m := _RE_HEAP_FIN.search(line):r.heap_final    = float(m.group(1))
        if m := _RE_FRAG.search(line):    r.frag_pct      = float(m.group(1))

        if in_time_section:
            if m := _RE_TIME_MIN.search(line): time_mins.append(float(m.group(1)))
            if m := _RE_TIME_AVG.search(line): time_avgs.append(float(m.group(1)))
            if m := _RE_TIME_MAX.search(line): time_maxs.append(float(m.group(1)))

        if in_heap_used_section:
            if m := _RE_HEAP_MIN.search(line): heap_mins.append(float(m.group(1)))
            if m := _RE_HEAP_AVG.search(line): heap_avgs.append(float(m.group(1)))
            if m := _RE_HEAP_MAX.search(line): heap_maxs.append(float(m.group(1)))

    if time_mins: r.time_min = time_mins[0]
    if time_avgs: r.time_avg = time_avgs[0]
    if time_maxs: r.time_max = time_maxs[0]
    if heap_mins: r.heap_used_min = heap_mins[0]
    if heap_avgs: r.heap_used_avg = heap_avgs[0]
    if heap_maxs: r.heap_used_max = heap_maxs[0]

    return r

def save(fig: plt.Figure, name: str) -> None:
    out = RESULTS_DIR / name
    fig.savefig(out, bbox_inches="tight", facecolor=fig.get_facecolor())
    print(f"  [OK] {out.relative_to(SCRIPT_DIR.parent)}")
    plt.close(fig)


def grouped_bars(ax, labels, vals_a, vals_b, unit="", colors=(COLOR_A, COLOR_B),
                 label_a=LABEL_A, label_b=LABEL_B):
    x = np.arange(len(labels))
    w = 0.35
    bars_a = ax.bar(x - w/2, vals_a, w, color=colors[0], alpha=0.9, label=label_a)
    bars_b = ax.bar(x + w/2, vals_b, w, color=colors[1], alpha=0.9, label=label_b)
    ax.set_xticks(x)
    ax.set_xticklabels(labels)
    ax.legend()
    for bar in (*bars_a, *bars_b):
        h = bar.get_height()
        ax.text(bar.get_x() + bar.get_width()/2, h + abs(h)*0.01,
                f"{h:,.0f}{unit}", ha="center", va="bottom", fontsize=9, color=COLOR_FG)
    return bars_a, bars_b


def pct_diff(a: float, b: float) -> str:
    if a == 0:
        return "N/A"
    d = (b - a) / abs(a) * 100
    sign = "+" if d > 0 else ""
    return f"{sign}{d:.1f}%"


def insight_box(ax, lines: list[str]) -> None:
    txt = "\n".join(lines)
    ax.text(0.5, 0.5, txt, transform=ax.transAxes,
            ha="center", va="center", fontsize=10,
            color=COLOR_FG, family="monospace",
            bbox=dict(boxstyle="round,pad=0.6", facecolor="#161B22",
                      edgecolor=COLOR_GRID, linewidth=1.2))
    ax.axis("off")

def chart_latency_per_request(a: BenchResult, b: BenchResult) -> None:
    fig, axes = plt.subplots(2, 1, figsize=(14, 8), sharex=False)
    fig.suptitle("Latency per Request", fontsize=16, fontweight="bold", y=1.01)

    for ax, res, color in zip(axes, [a, b], [COLOR_A, COLOR_B]):
        if not res.req_times_ms:
            ax.set_title(f"{res.label}  –  no data")
            continue
        x = res.req_numbers
        y = res.req_times_ms
        avg = np.mean(y)
        ax.plot(x, y, color=color, linewidth=1.2, alpha=0.8, label="Request time")
        ax.axhline(avg, color="white", linestyle="--", linewidth=0.9, alpha=0.7,
                   label=f"Avg  {avg:.1f} ms")
        ax.fill_between(x, y, avg, where=[v > avg for v in y],
                        alpha=0.15, color="red", label="Above avg")
        ax.fill_between(x, y, avg, where=[v <= avg for v in y],
                        alpha=0.15, color="lime", label="Below avg")
        ax.set_title(res.label, fontsize=13, fontweight="bold", color=color)
        ax.set_ylabel("Time (ms)")
        ax.set_xlabel("Request #")
        ax.legend(fontsize=9)

    plt.tight_layout()
    save(fig, "01_latency_per_request.png")


def chart_latency_stats(a: BenchResult, b: BenchResult) -> None:
    fig, (ax, ax_ins) = plt.subplots(1, 2, figsize=(14, 6),
                                      gridspec_kw={"width_ratios": [3, 1]})
    fig.suptitle("Latency Statistics (ms)", fontsize=16, fontweight="bold")

    labels = ["Min", "Avg", "Max"]
    vals_a = [a.time_min, a.time_avg, a.time_max]
    vals_b = [b.time_min, b.time_avg, b.time_max]
    grouped_bars(ax, labels, vals_a, vals_b, unit=" ms")
    ax.set_ylabel("Time (ms)")

    winner = LABEL_A if a.time_avg <= b.time_avg else LABEL_B
    insight_box(ax_ins, [
        "─── Latency Insights ───",
        "",
        f"  Min   A: {a.time_min:.0f} ms  B: {b.time_min:.0f} ms",
        f"  Avg   A: {a.time_avg:.1f} ms  B: {b.time_avg:.1f} ms",
        f"  Max   A: {a.time_max:.0f} ms  B: {b.time_max:.0f} ms",
        "",
        f"  Avg diff: {pct_diff(a.time_avg, b.time_avg)}",
        f"  (B vs A)",
        "",
        f"  ✦ Faster avg: {winner}",
    ])

    plt.tight_layout()
    save(fig, "02_latency_stats.png")

def chart_heap_per_request(a: BenchResult, b: BenchResult) -> None:
    fig, axes = plt.subplots(2, 1, figsize=(14, 9), sharex=False)
    fig.suptitle("Heap Consumed per Request (vs Total Available Heap)",
                 fontsize=16, fontweight="bold")

    for ax, res, color in zip(axes, [a, b], [COLOR_A, COLOR_B]):
        if not res.heap_used or not res.heap_before:
            ax.set_title(f"{res.label}  -  no data"); continue

        ok_indices = [i for i in range(len(res.req_ok)) if res.req_ok[i]]
        ok_x   = [res.req_numbers[i] for i in ok_indices]
        ok_bytes = [res.heap_used[i] for i in ok_indices]

        avg_bytes = np.mean(ok_bytes) if ok_bytes else 0.0
        max_bytes = max(ok_bytes)     if ok_bytes else 0.0

        initial_bytes = res.heap_initial if res.heap_initial > 0 else (res.heap_before[ok_indices[0]] if ok_indices else 0)

        bars = ax.bar(ok_x, ok_bytes, color=color, alpha=0.75, width=0.8)

        ax.axhline(avg_bytes, color="white", linestyle="--", linewidth=1.0,
                   label=f"Avg {avg_bytes:,.0f} bytes")

        ax.axhline(max_bytes, color="#FF5252", linestyle=":", linewidth=0.8,
                   label=f"Peak {max_bytes:,.0f} bytes")

        ax.set_title(res.label, fontsize=13, fontweight="bold", color=color)
        ax.set_ylabel("Heap consumed (bytes)", fontsize=10)
        ax.set_xlabel("Request #", fontsize=10)
        
        ax.set_ylim(0, initial_bytes * 1.05 if initial_bytes > 0 else max_bytes * 1.5)
        ax.yaxis.set_major_formatter(plt.FuncFormatter(lambda v, _: f"{v:,.0f} B"))
        ax.legend(fontsize=9)

        initial_kb = initial_bytes / 1024
        ax.text(0.99, 0.97,
                f"Total Available: {initial_kb:.1f} KB",
                transform=ax.transAxes, ha="right", va="top",
                fontsize=8, color=COLOR_FG, alpha=0.7)

    plt.tight_layout()
    save(fig, "03_heap_used_per_request.png")

def chart_heap_stats(a: BenchResult, b: BenchResult) -> None:
    fig, (ax, ax_ins) = plt.subplots(1, 2, figsize=(14, 6),
                                      gridspec_kw={"width_ratios": [3, 1]})
    fig.suptitle("Heap Consumed per Request  (bytes)", fontsize=16, fontweight="bold")

    labels = ["Min", "Avg", "Max"]
    vals_a = [a.heap_used_min, a.heap_used_avg, a.heap_used_max]
    vals_b = [b.heap_used_min, b.heap_used_avg, b.heap_used_max]
    grouped_bars(ax, labels, vals_a, vals_b, unit=" B")
    ax.set_ylabel("Bytes")

    winner = LABEL_A if a.heap_used_avg <= b.heap_used_avg else LABEL_B
    insight_box(ax_ins, [
        "─── Heap Insights ───",
        "",
        f"  Min   A: {a.heap_used_min:.0f} B   B: {b.heap_used_min:.0f} B",
        f"  Avg   A: {a.heap_used_avg:.0f} B   B: {b.heap_used_avg:.0f} B",
        f"  Max   A: {a.heap_used_max:.0f} B   B: {b.heap_used_max:.0f} B",
        "",
        f"  Avg diff: {pct_diff(a.heap_used_avg, b.heap_used_avg)}",
        f"  (B vs A)",
        "",
        f"  ✦ Lower heap: {winner}",
    ])

    plt.tight_layout()
    save(fig, "04_heap_stats.png")

def chart_system_heap(a: BenchResult, b: BenchResult) -> None:
    fig, (ax, ax_ins) = plt.subplots(1, 2, figsize=(11, 5),
                                      gridspec_kw={"width_ratios": [2, 1]})
    fig.suptitle("System Minimum Free Heap  (esp_get_minimum_free_heap_size)",
                 fontsize=14, fontweight="bold")

    vals  = [a.sys_heap_min / 1024, b.sys_heap_min / 1024]
    bars  = ax.bar([LABEL_A, LABEL_B], vals, color=[COLOR_A, COLOR_B], alpha=0.9, width=0.4)
    for bar, v in zip(bars, vals):
        ax.text(bar.get_x() + bar.get_width()/2, v + 0.3,
                f"{v:.1f} KB", ha="center", va="bottom", fontsize=10, color=COLOR_FG)
    ax.set_ylabel("Free heap (KB)")
    ax.set_ylim(0, max(vals) * 1.25)

    winner = LABEL_A if a.sys_heap_min >= b.sys_heap_min else LABEL_B
    diff_kb = abs(a.sys_heap_min - b.sys_heap_min) / 1024
    insight_box(ax_ins, [
        "─── System Heap ───",
        "",
        f"  A: {a.sys_heap_min/1024:.1f} KB",
        f"  B: {b.sys_heap_min/1024:.1f} KB",
        "",
        f"  Δ  {diff_kb:.1f} KB",
        "",
        f"  ✦ More free: {winner}",
        "  (higher = better)",
    ])

    plt.tight_layout()
    save(fig, "05_system_heap.png")

def chart_fragmentation(a: BenchResult, b: BenchResult) -> None:
    fig, (ax, ax_ins) = plt.subplots(1, 2, figsize=(11, 5),
                                      gridspec_kw={"width_ratios": [2, 1]})
    fig.suptitle("Heap Fragmentation Estimate  (after all requests)",
                 fontsize=14, fontweight="bold")

    vals  = [a.frag_pct, b.frag_pct]
    bars  = ax.bar([LABEL_A, LABEL_B], vals, color=[COLOR_A, COLOR_B], alpha=0.9, width=0.4)
    for bar, v in zip(bars, vals):
        ax.text(bar.get_x() + bar.get_width()/2, v + 0.02,
                f"{v:.2f}%", ha="center", va="bottom", fontsize=10, color=COLOR_FG)
    ax.set_ylabel("Fragmentation (%)")
    ax.set_ylim(0, max(max(vals) * 1.4, 1))

    winner = LABEL_A if a.frag_pct <= b.frag_pct else LABEL_B
    insight_box(ax_ins, [
        "─── Fragmentation ───",
        "",
        f"  A: {a.frag_pct:.2f}%",
        f"  B: {b.frag_pct:.2f}%",
        "",
        f"  Δ  {abs(a.frag_pct - b.frag_pct):.2f}%",
        "",
        f"  ✦ Less frag: {winner}",
        "  (lower = better)",
    ])

    plt.tight_layout()
    save(fig, "06_fragmentation.png")

def chart_success_rate(a: BenchResult, b: BenchResult) -> None:
    fig, axes = plt.subplots(1, 2, figsize=(10, 5))
    fig.suptitle("Success Rate", fontsize=16, fontweight="bold")

    for ax, res, color in zip(axes, [a, b], [COLOR_A, COLOR_B]):
        total = res.total or (res.success + res.failed) or 1
        ok, fail = res.success, res.failed
        pct = ok / total * 100
        wedge_colors = [color, "#3D4451"]
        wedges, _ = ax.pie(
            [ok, fail],
            colors=wedge_colors,
            startangle=90,
            wedgeprops=dict(width=0.45, edgecolor=COLOR_BG, linewidth=2),
        )
        ax.text(0, 0, f"{pct:.1f}%", ha="center", va="center",
                fontsize=20, fontweight="bold", color=COLOR_FG)
        ax.set_title(res.label, fontsize=12, fontweight="bold", color=color, pad=14)
        ax.legend(
            [mpatches.Patch(color=wedge_colors[0]), mpatches.Patch(color=wedge_colors[1])],
            [f"OK ({ok})", f"FAIL ({fail})"],
            loc="lower center", ncol=2, fontsize=9,
        )

    plt.tight_layout()
    save(fig, "07_success_rate.png")

def chart_dashboard(a: BenchResult, b: BenchResult) -> None:
    fig = plt.figure(figsize=(20, 14), facecolor=COLOR_BG)
    fig.suptitle("ESP32 HTTP Benchmark  —  Full Summary Dashboard",
                 fontsize=20, fontweight="bold", y=1.01)

    gs = gridspec.GridSpec(3, 3, figure=fig, hspace=0.55, wspace=0.4)

    for col, (res, color) in enumerate([(a, COLOR_A), (b, COLOR_B)]):
        ax = fig.add_subplot(gs[0, col])
        if res.req_times_ms:
            avg = np.mean(res.req_times_ms)
            ax.plot(res.req_numbers, res.req_times_ms,
                    color=color, linewidth=1.0, alpha=0.85)
            ax.axhline(avg, color="white", linestyle="--", linewidth=0.8,
                       label=f"Avg {avg:.1f} ms")
            ax.legend(fontsize=8)
        ax.set_title(f"Latency — {res.label}", fontsize=10, color=color)
        ax.set_xlabel("Request #", fontsize=8)
        ax.set_ylabel("ms", fontsize=8)

    ax = fig.add_subplot(gs[0, 2])
    labels = ["Min", "Avg", "Max"]
    x = np.arange(3); w = 0.35
    ax.bar(x - w/2, [a.time_min, a.time_avg, a.time_max], w, color=COLOR_A,
           alpha=0.9, label=LABEL_A)
    ax.bar(x + w/2, [b.time_min, b.time_avg, b.time_max], w, color=COLOR_B,
           alpha=0.9, label=LABEL_B)
    ax.set_xticks(x); ax.set_xticklabels(labels, fontsize=9)
    ax.set_title("Latency Stats (ms)", fontsize=10)
    ax.legend(fontsize=8)

    for col, (res, color) in enumerate([(a, COLOR_A), (b, COLOR_B)]):
        ax = fig.add_subplot(gs[1, col])
        ok_indices = [i for i in range(len(res.req_ok)) if res.req_ok[i]]
        ok_x   = [res.req_numbers[i] for i in ok_indices]
        ok_bytes = [res.heap_used[i] for i in ok_indices]
        
        if ok_bytes:
            avg_bytes = np.mean(ok_bytes)
            max_bytes = max(ok_bytes)
            initial_bytes = res.heap_initial if res.heap_initial > 0 else (res.heap_before[ok_indices[0]] if ok_indices else 0)
            
            ax.bar(ok_x, ok_bytes, color=color, alpha=0.7, width=0.8)
            ax.axhline(avg_bytes, color="white", linestyle="--", linewidth=0.8,
                       label=f"Avg {avg_bytes:,.0f} B")
            ax.set_ylim(0, initial_bytes * 1.05 if initial_bytes > 0 else max_bytes * 1.5)
            ax.yaxis.set_major_formatter(plt.FuncFormatter(lambda v, _: f"{v:,.0f} B"))
            ax.legend(fontsize=7)
            
            initial_kb = initial_bytes / 1024
            ax.text(0.99, 0.97, f"Avail: {initial_kb:.1f} KB",
                    transform=ax.transAxes, ha="right", va="top",
                    fontsize=7, color=COLOR_FG, alpha=0.7)
            
        ax.set_title(f"Heap Used -- {res.label}", fontsize=10, color=color)
        ax.set_xlabel("Request #", fontsize=8)
        ax.set_ylabel("Bytes", fontsize=8)


    ax = fig.add_subplot(gs[1, 2])
    ax.axis("off")
    ax_a = fig.add_axes([0.69, 0.36, 0.13, 0.22])
    ax_b = fig.add_axes([0.84, 0.36, 0.13, 0.22])

    for mini_ax, res, color in [(ax_a, a, COLOR_A), (ax_b, b, COLOR_B)]:
        total = res.total or (res.success + res.failed) or 1
        mini_ax.pie([res.success, res.failed],
                    colors=[color, "#3D4451"],
                    startangle=90,
                    wedgeprops=dict(width=0.45, edgecolor=COLOR_BG, linewidth=1.5))
        pct = res.success / total * 100
        mini_ax.text(0, 0, f"{pct:.0f}%", ha="center", va="center",
                     fontsize=9, fontweight="bold", color=COLOR_FG)
        mini_ax.set_title(res.label[:16], fontsize=7, color=color, pad=4)
        mini_ax.set_facecolor(COLOR_BG)

    ax = fig.add_subplot(gs[2, 0])
    vals = [a.sys_heap_min/1024, b.sys_heap_min/1024]
    bars = ax.bar([LABEL_A[:16], LABEL_B[:16]], vals,
                  color=[COLOR_A, COLOR_B], alpha=0.9, width=0.4)
    for bar, v in zip(bars, vals):
        ax.text(bar.get_x() + bar.get_width()/2, v + 0.2,
                f"{v:.1f}KB", ha="center", va="bottom", fontsize=8, color=COLOR_FG)
    ax.set_title("System Min Free Heap", fontsize=10)
    ax.set_ylabel("KB", fontsize=8)
    ax.set_ylim(0, max(vals) * 1.3)
    ax.tick_params(axis="x", labelsize=7)

    ax = fig.add_subplot(gs[2, 1])
    vals_f = [a.frag_pct, b.frag_pct]
    bars = ax.bar([LABEL_A[:16], LABEL_B[:16]], vals_f,
                  color=[COLOR_A, COLOR_B], alpha=0.9, width=0.4)
    for bar, v in zip(bars, vals_f):
        ax.text(bar.get_x() + bar.get_width()/2, v + 0.01,
                f"{v:.2f}%", ha="center", va="bottom", fontsize=8, color=COLOR_FG)
    ax.set_title("Heap Fragmentation", fontsize=10)
    ax.set_ylabel("%", fontsize=8)
    ax.set_ylim(0, max(max(vals_f)*1.4, 0.5))
    ax.tick_params(axis="x", labelsize=7)

    ax = fig.add_subplot(gs[2, 2])
    lat_winner   = LABEL_A if a.time_avg <= b.time_avg else LABEL_B
    heap_winner  = LABEL_A if a.heap_used_avg <= b.heap_used_avg else LABEL_B
    frag_winner  = LABEL_A if a.frag_pct <= b.frag_pct else LABEL_B
    shwinner     = LABEL_A if a.sys_heap_min >= b.sys_heap_min else LABEL_B
    insight_box(ax, [
        "─── Key Insights ───",
        "",
        f"  Avg latency  A:{a.time_avg:.1f}ms  B:{b.time_avg:.1f}ms",
        f"  → Faster: {lat_winner[:18]}",
        "",
        f"  Avg heap used  A:{a.heap_used_avg:.0f}B  B:{b.heap_used_avg:.0f}B",
        f"  → Lower: {heap_winner[:20]}",
        "",
        f"  Fragmentation  A:{a.frag_pct:.2f}%  B:{b.frag_pct:.2f}%",
        f"  → Less fragmented: {frag_winner[:13]}",
        "",
        f"  Sys free heap  A:{a.sys_heap_min//1024}KB  B:{b.sys_heap_min//1024}KB",
        f"  → More free: {shwinner[:18]}",
    ])

    save(fig, "08_dashboard.png")

def main() -> None:
    print("\n ESP32 Benchmark Analyzer")
    print("  " + "-" * 46)

    a = parse_result(BENCH_A_RESULT, LABEL_A)
    b = parse_result(BENCH_B_RESULT, LABEL_B)

    print(f"\n  {LABEL_A}")
    print(f"    Requests: {a.total}  |  OK: {a.success}  |  FAIL: {a.failed}")
    print(f"    Latency avg: {a.time_avg:.1f} ms  |  Heap avg: {a.heap_used_avg:.0f} B")

    print(f"\n  {LABEL_B}")
    print(f"    Requests: {b.total}  |  OK: {b.success}  |  FAIL: {b.failed}")
    print(f"    Latency avg: {b.time_avg:.1f} ms  |  Heap avg: {b.heap_used_avg:.0f} B")

    print(f"\n  Generating charts → {RESULTS_DIR.relative_to(SCRIPT_DIR.parent)}\n")

    chart_latency_per_request(a, b)
    chart_latency_stats(a, b)
    chart_heap_per_request(a, b)
    chart_heap_stats(a, b)
    chart_system_heap(a, b)
    chart_fragmentation(a, b)
    chart_success_rate(a, b)
    chart_dashboard(a, b)

    print("  Done! All charts saved.\n")


if __name__ == "__main__":
    main()
