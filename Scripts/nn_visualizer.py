#!/usr/bin/env python3
"""Visualize Eleanor's NNUE network (`nnue.bin`) as a standalone HTML atlas.

This tool is purpose-built for Eleanor's network layout:
- optional 48-byte header (24 int16 values)
- accumulator weights: 768 features x 1536 hidden neurons (int16, feature-major)
- accumulator biases: 1536 int16
- output weights: 8 x (2 * 1536) int16
- output biases: 8 int16

Example:
    python3 Scripts/nn_visualizer.py --nnue nnue.bin --output nnue_visualizer.html
"""

from __future__ import annotations

import argparse
import html
import math
from array import array
from dataclasses import dataclass
from pathlib import Path


INPUT_SIZE = 768
HL_SIZE = 1536
OUTPUT_BUCKETS = 8
HEADER_I16_COUNT = 24
QA = 255
QB = 64
SCALE = 400

PIECE_LABELS = ("P", "N", "B", "R", "Q", "K")
PALETTE_STOPS = (
    (0.00, (0, 0, 4)),
    (0.12, (28, 12, 68)),
    (0.25, (79, 18, 123)),
    (0.38, (129, 37, 129)),
    (0.52, (181, 54, 122)),
    (0.68, (229, 80, 56)),
    (0.82, (248, 137, 12)),
    (0.92, (252, 196, 52)),
    (1.00, (252, 255, 164)),
)
ZERO_RGB = (0, 0, 0)


@dataclass
class NnueData:
    header: list[int]
    acc_weights: list[int]  # flattened [INPUT_SIZE][HL_SIZE] (feature-major)
    acc_biases: list[int]  # [HL_SIZE]
    out_weights: list[int]  # flattened [OUTPUT_BUCKETS][2*HL_SIZE]
    out_biases: list[int]  # [OUTPUT_BUCKETS]


@dataclass
class Stats:
    minimum: int
    maximum: int
    mean: float
    mean_abs: float


@dataclass
class NeuronSummary:
    index: int
    minimum: int
    maximum: int
    mean: float
    mean_abs: float
    stddev: float
    span: int
    bias: int
    output_peak: int

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Create a colorful NNUE visualization as HTML/SVG.")
    parser.add_argument("--nnue", type=Path, default=Path("nnue.bin"), help="Path to Eleanor NNUE binary file.")
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("nnue_visualizer.html"),
        help="Output HTML path.",
    )
    parser.add_argument(
        "--pool",
        default="128x96",
        help="Pooling grid for the full accumulator matrix: <cols>x<rows> (default: 128x96).",
    )
    parser.add_argument(
        "--bucket-cols",
        type=int,
        default=96,
        help="Pooled columns for each output bucket overview (rows are fixed to 2).",
    )
    parser.add_argument(
        "--neurons",
        type=int,
        default=24,
        help="How many hidden neurons to show in the board atlas (default: 24).",
    )
    parser.add_argument(
        "--sort",
        choices=("range", "meanabs", "stddev"),
        default="range",
        help="How to rank atlas neurons (default: range).",
    )
    return parser.parse_args()


def load_nnue(path: Path) -> NnueData:
    raw = array("h")
    with path.open("rb") as handle:
        raw.fromfile(handle, path.stat().st_size // 2)

    expected = HL_SIZE * INPUT_SIZE + HL_SIZE + OUTPUT_BUCKETS * (2 * HL_SIZE) + OUTPUT_BUCKETS
    if len(raw) == expected:
        offset = 0
    elif len(raw) == expected + HEADER_I16_COUNT:
        offset = HEADER_I16_COUNT
    else:
        raise ValueError(
            f"Unexpected nnue.bin size: found {len(raw)} int16 values, expected {expected} or {expected + HEADER_I16_COUNT}."
        )

    header = list(raw[:offset])
    i = offset

    acc_count = INPUT_SIZE * HL_SIZE
    acc_weights = list(raw[i : i + acc_count])
    i += acc_count

    acc_biases = list(raw[i : i + HL_SIZE])
    i += HL_SIZE

    out_count = OUTPUT_BUCKETS * (2 * HL_SIZE)
    out_weights = list(raw[i : i + out_count])
    i += out_count

    out_biases = list(raw[i : i + OUTPUT_BUCKETS])

    return NnueData(
        header=header,
        acc_weights=acc_weights,
        acc_biases=acc_biases,
        out_weights=out_weights,
        out_biases=out_biases,
    )


def compute_stats(values: list[int]) -> Stats:
    if not values:
        return Stats(minimum=0, maximum=0, mean=0.0, mean_abs=0.0)
    total = 0
    total_abs = 0
    vmin = values[0]
    vmax = values[0]
    for value in values:
        total += value
        total_abs += abs(value)
        vmin = min(vmin, value)
        vmax = max(vmax, value)
    count = len(values)
    return Stats(minimum=vmin, maximum=vmax, mean=total / count, mean_abs=total_abs / count)


def parse_pool(pool_text: str) -> tuple[int, int]:
    pieces = pool_text.lower().split("x")
    if len(pieces) != 2:
        raise ValueError("--pool must look like 128x96")
    cols = int(pieces[0])
    rows = int(pieces[1])
    if cols <= 0 or rows <= 0:
        raise ValueError("pool dimensions must be positive")
    return cols, rows


def pooled_average(values: list[int] | list[float], src_rows: int, src_cols: int, out_rows: int, out_cols: int) -> list[float]:
    out: list[float] = []
    for row in range(out_rows):
        r0 = (row * src_rows) // out_rows
        r1 = ((row + 1) * src_rows) // out_rows
        if r1 <= r0:
            r1 = min(r0 + 1, src_rows)
        for col in range(out_cols):
            c0 = (col * src_cols) // out_cols
            c1 = ((col + 1) * src_cols) // out_cols
            if c1 <= c0:
                c1 = min(c0 + 1, src_cols)

            total = 0.0
            count = 0
            for rr in range(r0, r1):
                base = rr * src_cols
                for cc in range(c0, c1):
                    total += values[base + cc]
                    count += 1
            out.append(total / max(count, 1))
    return out


def palette_color(position: float) -> tuple[int, int, int]:
    x = max(0.0, min(1.0, position))
    for (left_pos, left_rgb), (right_pos, right_rgb) in zip(PALETTE_STOPS, PALETTE_STOPS[1:]):
        if x <= right_pos:
            span = right_pos - left_pos
            t = 0.0 if span <= 0 else (x - left_pos) / span
            return tuple(int(left + (right - left) * t) for left, right in zip(left_rgb, right_rgb))
    return PALETTE_STOPS[-1][1]


def css_rgb(rgb: tuple[int, int, int]) -> str:
    return f"rgb({rgb[0]},{rgb[1]},{rgb[2]})"


def positive_color_for_value(value: float, minimum: float, maximum: float) -> str:
    if maximum <= minimum:
        t = 0.5
    else:
        t = (value - minimum) / (maximum - minimum)
    return css_rgb(palette_color(t))


def signed_color_for_value(value: float, bound: float) -> str:
    if bound <= 0:
        return css_rgb(ZERO_RGB)
    normalized = max(-1.0, min(1.0, value / bound))
    if abs(normalized) < 1e-12:
        return css_rgb(ZERO_RGB)

    base = palette_color(abs(normalized))
    if normalized > 0:
        return css_rgb(base)

    # Swap warm inferno into a cooler blue-forward ramp for negative values.
    r, g, b = base
    return css_rgb((b, int(g * 0.84), max(r, 10)))


def percentile_bounds(values: list[float], low_quantile: float, high_quantile: float) -> tuple[float, float]:
    if not values:
        return 0.0, 0.0
    ordered = sorted(values)
    last = len(ordered) - 1
    low_index = max(0, min(last, int(last * low_quantile)))
    high_index = max(0, min(last, int(last * high_quantile)))
    return ordered[low_index], ordered[high_index]


def absolute_percentile_bound(values: list[int] | list[float], quantile: float) -> float:
    if not values:
        return 0.0
    ordered = sorted(abs(value) for value in values)
    index = max(0, min(len(ordered) - 1, int((len(ordered) - 1) * quantile)))
    return max(ordered[index], 1e-9)


def summarize_neurons(data: NnueData) -> list[NeuronSummary]:
    output_peaks = [0] * HL_SIZE
    for bucket in range(OUTPUT_BUCKETS):
        base = bucket * (2 * HL_SIZE)
        for neuron in range(HL_SIZE):
            stm = abs(data.out_weights[base + neuron])
            nstm = abs(data.out_weights[base + HL_SIZE + neuron])
            output_peaks[neuron] = max(output_peaks[neuron], stm, nstm)

    summaries: list[NeuronSummary] = []
    for neuron in range(HL_SIZE):
        first = data.acc_weights[neuron]
        total = 0
        total_abs = 0
        total_sq = 0
        vmin = first
        vmax = first
        for feature in range(INPUT_SIZE):
            value = data.acc_weights[feature * HL_SIZE + neuron]
            total += value
            total_abs += abs(value)
            total_sq += value * value
            vmin = min(vmin, value)
            vmax = max(vmax, value)
        mean = total / INPUT_SIZE
        variance = max((total_sq / INPUT_SIZE) - (mean * mean), 0.0)
        summaries.append(
            NeuronSummary(
                index=neuron,
                minimum=vmin,
                maximum=vmax,
                mean=mean,
                mean_abs=total_abs / INPUT_SIZE,
                stddev=math.sqrt(variance),
                span=vmax - vmin,
                bias=data.acc_biases[neuron],
                output_peak=output_peaks[neuron],
            )
        )
    return summaries


def sort_neuron_summaries(summaries: list[NeuronSummary], mode: str, count: int) -> list[NeuronSummary]:
    if mode == "meanabs":
        key = lambda summary: (summary.mean_abs, summary.span, summary.stddev)
    elif mode == "stddev":
        key = lambda summary: (summary.stddev, summary.span, summary.mean_abs)
    else:
        key = lambda summary: (summary.span, summary.mean_abs, summary.stddev)
    return sorted(summaries, key=key, reverse=True)[: max(0, min(count, HL_SIZE))]


def extract_neuron_planes(acc_weights: list[int], neuron: int) -> list[list[int]]:
    planes: list[list[int]] = []
    for plane_index in range(12):
        plane: list[int] = []
        feature_start = plane_index * 64
        for square in range(64):
            feature = feature_start + square
            plane.append(acc_weights[feature * HL_SIZE + neuron])
        planes.append(plane)
    return planes


def feature_energy(acc_weights: list[int]) -> list[float]:
    energies: list[float] = []
    for feature in range(INPUT_SIZE):
        base = feature * HL_SIZE
        total_abs = 0
        for neuron in range(HL_SIZE):
            total_abs += abs(acc_weights[base + neuron])
        energies.append(total_abs / HL_SIZE)
    return energies


def make_piece_board_svg(
    values: list[int] | list[float],
    minimum: float,
    maximum: float,
    label: str,
    title_label: str,
    *,
    signed_scale: bool,
    cell: int,
    compact: bool,
    show_label: bool,
) -> str:
    size = cell * 8
    rects: list[str] = []
    if signed_scale:
        bound = max(abs(minimum), abs(maximum), 1e-9)
        color_fn = lambda value: signed_color_for_value(value, bound)
    else:
        color_fn = lambda value: positive_color_for_value(value, minimum, maximum)
    for rank_from_top in range(8):
        board_rank = 7 - rank_from_top
        y = rank_from_top * cell
        for file_index in range(8):
            square = board_rank * 8 + file_index
            x = file_index * cell
            color = color_fn(values[square])
            rects.append(f'<rect x="{x}" y="{y}" width="{cell}" height="{cell}" fill="{color}" />')

    figure_class = "plane plane-compact" if compact else "plane"
    caption_html = f"<figcaption>{html.escape(label)}</figcaption>" if show_label else ""

    return (
        f'<figure class="{figure_class}" title="{html.escape(title_label)}">'
        + caption_html
        + f'<svg viewBox="0 0 {size} {size}" width="100%" height="100%" '
        + f'xmlns="http://www.w3.org/2000/svg" shape-rendering="crispEdges">'
        + f'<rect x="0" y="0" width="{size}" height="{size}" fill="#000000" />'
        + "".join(rects)
        + "</svg></figure>"
    )


def make_plane_atlas(
    values: list[int] | list[float],
    minimum: float,
    maximum: float,
    *,
    signed_scale: bool,
    compact: bool = False,
    show_piece_labels: bool = True,
    show_row_labels: bool = True,
) -> str:
    rows: list[str] = []
    for side_index, side_label in enumerate(("Plane A", "Plane B")):
        boards: list[str] = []
        for piece_index, piece_label in enumerate(PIECE_LABELS):
            plane_index = side_index * 6 + piece_index
            boards.append(
                make_piece_board_svg(
                    values[plane_index * 64 : (plane_index + 1) * 64],
                    minimum,
                    maximum,
                    piece_label,
                    f"{side_label} {piece_label}",
                    signed_scale=signed_scale,
                    cell=4 if compact else 12,
                    compact=compact,
                    show_label=show_piece_labels,
                )
            )
        row_label_html = (
            f"<div class='atlas-row-label'>{html.escape(side_label)}</div>"
            if show_row_labels
            else ""
        )
        row_class = "atlas-row atlas-row-compact" if compact else "atlas-row"
        row_wrap_class = "atlas-row-wrap atlas-row-wrap-compact" if compact else "atlas-row-wrap"
        rows.append(
            f"<div class='{row_wrap_class}'>"
            + row_label_html
            + f"<div class='{row_class}'>{''.join(boards)}</div>"
            "</div>"
        )
    frame_class = "atlas-frame atlas-frame-compact" if compact else "atlas-frame"
    stack_class = "atlas-stack atlas-stack-compact" if compact else "atlas-stack"
    return f'<div class="{frame_class}"><div class="{stack_class}">' + "".join(rows) + "</div></div>"


def make_svg_heatmap(
    pooled: list[float],
    rows: int,
    cols: int,
    title: str,
    subtitle: str,
    pixel: int = 6,
    clip_quantiles: tuple[float, float] | None = None,
    signed_scale: bool = False,
) -> str:
    data_min = min(pooled) if pooled else 0.0
    data_max = max(pooled) if pooled else 0.0
    color_min = data_min
    color_max = data_max
    signed_bound = max(abs(data_min), abs(data_max), 1e-9)
    if signed_scale:
        if clip_quantiles is not None:
            signed_bound = absolute_percentile_bound(pooled, clip_quantiles[1])
        color_fn = lambda value: signed_color_for_value(value, signed_bound)
    else:
        if clip_quantiles is not None:
            color_min, color_max = percentile_bounds(pooled, clip_quantiles[0], clip_quantiles[1])
            if color_max <= color_min:
                color_min, color_max = data_min, data_max
        color_fn = lambda value: positive_color_for_value(value, color_min, color_max)
    width = cols * pixel
    height = rows * pixel

    rects: list[str] = []
    index = 0
    for row in range(rows):
        y = row * pixel
        for col in range(cols):
            x = col * pixel
            color = color_fn(pooled[index])
            rects.append(f'<rect x="{x}" y="{y}" width="{pixel}" height="{pixel}" fill="{color}" />')
            index += 1

    range_label = f"data {data_min:.2f} .. {data_max:.2f}"
    if signed_scale:
        range_label += f" | signed palette ±{signed_bound:.2f}"
    elif clip_quantiles is not None:
        range_label += f" | palette {color_min:.2f} .. {color_max:.2f}"

    return (
        f'<section class="panel heatmap-panel">'
        f'<h3>{html.escape(title)}</h3>'
        f'<p class="subtitle">{html.escape(subtitle)} | {range_label}</p>'
        f'<svg viewBox="0 0 {width} {height}" width="100%" height="auto" '
        f'xmlns="http://www.w3.org/2000/svg" preserveAspectRatio="none" shape-rendering="crispEdges">'
        + "".join(rects)
        + "</svg></section>"
    )


def make_bucket_biases(values: list[int]) -> str:
    tiles: list[str] = []
    minimum = min(values) if values else 0
    maximum = max(values) if values else 0
    for bucket, value in enumerate(values):
        color = positive_color_for_value(value, minimum, maximum)
        tiles.append(
            "<div class='bias-tile'>"
            f"<span class='bias-label'>Bucket {bucket}</span>"
            f"<span class='bias-chip' style='background:{color}'></span>"
            f"<span class='bias-value'>{value}</span>"
            "</div>"
        )
    return (
        "<section class='panel'>"
        "<h3>Output Biases</h3>"
        "<p class='subtitle'>Lighter tiles are higher bucket biases.</p>"
        "<div class='bias-grid'>"
        + "".join(tiles)
        + "</div></section>"
    )


def summarize_stats(name: str, stats: Stats) -> str:
    return (
        f"<tr><td>{html.escape(name)}</td><td>{stats.minimum}</td><td>{stats.maximum}</td>"
        f"<td>{stats.mean:.3f}</td><td>{stats.mean_abs:.3f}</td></tr>"
    )


def make_neuron_card(data: NnueData, summary: NeuronSummary) -> str:
    planes = extract_neuron_planes(data.acc_weights, summary.index)
    plane_values = [value for plane in planes for value in plane]
    signed_bound = absolute_percentile_bound(plane_values, 0.98)
    atlas = make_plane_atlas(
        plane_values,
        -signed_bound,
        signed_bound,
        signed_scale=True,
    )
    return (
        "<article class='neuron-card'>"
        "<div class='neuron-header'>"
        f"<div><h3>Neuron {summary.index}</h3><p class='subtitle'>bias {summary.bias} | output |w|max {summary.output_peak}</p></div>"
        f"<div class='score-pill'>range {summary.span}<br /><span>mean |w| {summary.mean_abs:.1f}</span><br /><span>palette ±{signed_bound:.1f}</span></div>"
        "</div>"
        + atlas
        + (
            "<div class='neuron-meta'>"
            f"<span>min {summary.minimum}</span>"
            f"<span>max {summary.maximum}</span>"
            f"<span>mean {summary.mean:.2f}</span>"
            f"<span>stddev {summary.stddev:.2f}</span>"
            "</div>"
        )
        + "</article>"
    )


def make_neuron_wall(data: NnueData, summaries: list[NeuronSummary]) -> str:
    tiles: list[str] = []
    for summary in summaries:
        planes = extract_neuron_planes(data.acc_weights, summary.index)
        plane_values = [value for plane in planes for value in plane]
        signed_bound = absolute_percentile_bound(plane_values, 0.98)
        atlas = make_plane_atlas(
            plane_values,
            -signed_bound,
            signed_bound,
            signed_scale=True,
            compact=True,
            show_piece_labels=False,
            show_row_labels=False,
        )
        tiles.append(
            "<article class='wall-tile'>"
            + atlas
            + f"<div class='wall-meta'><span>#{summary.index}</span><span>±{signed_bound:.0f}</span></div>"
            + "</article>"
        )
    return "<div class='wall-grid'>" + "".join(tiles) + "</div>"


def build_html(
    data: NnueData,
    pool_cols: int,
    pool_rows: int,
    bucket_cols: int,
    neuron_count: int,
    sort_mode: str,
    source_path: Path,
) -> str:
    acc_stats = compute_stats(data.acc_weights)
    bias_stats = compute_stats(data.acc_biases)
    outw_stats = compute_stats(data.out_weights)
    outb_stats = compute_stats(data.out_biases)

    all_summaries = summarize_neurons(data)
    top_neurons = sort_neuron_summaries(all_summaries, sort_mode, neuron_count)
    neuron_wall = make_neuron_wall(data, top_neurons)
    neuron_cards = "".join(make_neuron_card(data, summary) for summary in top_neurons)

    energy = feature_energy(data.acc_weights)
    energy_atlas = make_plane_atlas(
        energy,
        min(energy),
        max(energy),
        signed_scale=False,
    )

    acc_pool = pooled_average(data.acc_weights, INPUT_SIZE, HL_SIZE, pool_rows, pool_cols)

    bucket_panels: list[str] = []
    for bucket in range(OUTPUT_BUCKETS):
        start = bucket * (2 * HL_SIZE)
        end = start + (2 * HL_SIZE)
        bucket_vals = data.out_weights[start:end]
        pooled = pooled_average(bucket_vals, 2, HL_SIZE, 2, bucket_cols)
        bucket_panels.append(
            make_svg_heatmap(
                pooled,
                rows=2,
                cols=bucket_cols,
                title=f"Output Bucket {bucket}",
                subtitle=f"STM and NSTM output weights pooled from 2x{HL_SIZE} to 2x{bucket_cols}",
                pixel=7,
                clip_quantiles=(0.08, 0.92),
                signed_scale=True,
            )
        )

    header_text = " ".join(str(value) for value in data.header) if data.header else "(none)"
    header_badge = "24 int16 header values present" if data.header else "no header detected"

    return f"""<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>Eleanor NNUE Visualizer</title>
  <style>
    :root {{
      color-scheme: dark;
      --bg-0: #08060d;
      --bg-1: #120d18;
      --panel: rgba(20, 16, 29, 0.88);
      --panel-2: rgba(25, 18, 35, 0.94);
      --line: rgba(255, 255, 255, 0.08);
      --text: #f7f1e8;
      --muted: #c7bccf;
      --muted-2: #9e92ad;
      --shadow: rgba(0, 0, 0, 0.38);
      --legend: linear-gradient(90deg, #9ad1fb 0%, #284978 20%, #07050a 50%, #5a167f 72%, #fcffa4 100%);
    }}
    * {{ box-sizing: border-box; }}
    body {{
      margin: 0;
      color: var(--text);
      font-family: "Avenir Next", "Segoe UI", "Helvetica Neue", sans-serif;
      background:
        radial-gradient(circle at top left, rgba(255, 141, 36, 0.18), transparent 26%),
        radial-gradient(circle at top right, rgba(143, 52, 224, 0.18), transparent 28%),
        linear-gradient(180deg, var(--bg-0) 0%, var(--bg-1) 48%, #0d0911 100%);
      min-height: 100vh;
    }}
    main {{ max-width: 1440px; margin: 0 auto; padding: 30px 22px 56px; }}
    h1, h2, h3, p {{ margin: 0; }}
    h1 {{
      font-size: clamp(2.1rem, 4vw, 3.8rem);
      line-height: 0.96;
      letter-spacing: -0.04em;
      margin-bottom: 10px;
    }}
    h2 {{ font-size: 1.15rem; margin-bottom: 8px; }}
    h3 {{ font-size: 1rem; margin-bottom: 6px; }}
    code {{
      font-family: "Iosevka", "JetBrains Mono", "SFMono-Regular", monospace;
      background: rgba(255, 255, 255, 0.06);
      border: 1px solid rgba(255, 255, 255, 0.08);
      border-radius: 999px;
      padding: 4px 10px;
      color: #fff4d8;
    }}
    .hero {{
      display: grid;
      gap: 18px;
      padding: 24px;
      background: linear-gradient(180deg, rgba(31, 22, 43, 0.92), rgba(17, 13, 25, 0.94));
      border: 1px solid var(--line);
      border-radius: 26px;
      box-shadow: 0 28px 80px var(--shadow);
      margin-bottom: 22px;
    }}
    .hero p {{ color: var(--muted); max-width: 82ch; line-height: 1.55; }}
    .hero-meta {{
      display: flex;
      flex-wrap: wrap;
      gap: 10px;
      align-items: center;
    }}
    .hero-meta span {{
      display: inline-flex;
      align-items: center;
      padding: 8px 12px;
      border-radius: 999px;
      background: rgba(255, 255, 255, 0.05);
      border: 1px solid rgba(255, 255, 255, 0.08);
      color: #f8eacb;
      font-size: 0.92rem;
    }}
    .legend-block {{
      display: grid;
      gap: 9px;
      max-width: 460px;
    }}
    .legend-bar {{
      height: 16px;
      border-radius: 999px;
      background: var(--legend);
      border: 1px solid rgba(255, 255, 255, 0.09);
      box-shadow: inset 0 0 0 1px rgba(0, 0, 0, 0.35);
    }}
    .legend-labels {{
      display: flex;
      justify-content: space-between;
      color: var(--muted);
      font-size: 0.9rem;
    }}
    .section {{
      margin-top: 28px;
    }}
    .section-copy {{
      display: grid;
      gap: 8px;
      margin-bottom: 14px;
    }}
    .section-copy p {{ color: var(--muted); line-height: 1.5; }}
    .panel {{
      background: var(--panel);
      border: 1px solid var(--line);
      border-radius: 22px;
      padding: 18px;
      box-shadow: 0 18px 40px rgba(0, 0, 0, 0.22);
    }}
    .atlas-frame {{
      background: #000000;
      border: 1px solid rgba(255, 255, 255, 0.04);
      border-radius: 18px;
      padding: 10px;
      box-shadow: inset 0 0 0 1px rgba(255, 255, 255, 0.02);
    }}
    .atlas-stack {{
      display: grid;
      gap: 10px;
    }}
    .atlas-frame-compact {{
      padding: 6px;
      border-radius: 14px;
    }}
    .atlas-stack-compact {{
      gap: 5px;
    }}
    .atlas-row-wrap {{
      display: grid;
      gap: 8px;
    }}
    .atlas-row-wrap-compact {{
      gap: 4px;
    }}
    .atlas-row-label {{
      font-family: "Iosevka", "JetBrains Mono", "SFMono-Regular", monospace;
      font-size: 0.76rem;
      letter-spacing: 0.16em;
      text-transform: uppercase;
      color: #b69ec2;
      padding-left: 4px;
    }}
    .atlas-row {{
      display: grid;
      grid-template-columns: repeat(6, minmax(0, 1fr));
      gap: 8px;
    }}
    .atlas-row-compact {{
      gap: 4px;
    }}
    .plane {{
      margin: 0;
      background: #000000;
      border-radius: 12px;
      padding: 4px;
      border: 1px solid rgba(255, 255, 255, 0.04);
      display: grid;
      gap: 4px;
    }}
    .plane figcaption {{
      font-family: "Iosevka", "JetBrains Mono", "SFMono-Regular", monospace;
      font-size: 0.82rem;
      letter-spacing: 0.06em;
      color: #ffd791;
      text-align: center;
    }}
    .plane svg {{
      width: 100%;
      height: auto;
      border-radius: 6px;
      display: block;
    }}
    .plane-compact {{
      padding: 2px;
      border-radius: 8px;
      gap: 0;
    }}
    .plane-compact svg {{
      border-radius: 4px;
    }}
    .atlas-note {{
      color: var(--muted);
      margin-top: 12px;
      line-height: 1.45;
    }}
    .wall-grid {{
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(240px, 1fr));
      gap: 14px;
      perspective: 1400px;
      perspective-origin: center top;
      isolation: isolate;
    }}
    .wall-tile {{
      display: grid;
      gap: 8px;
      padding: 10px;
      position: relative;
      background: linear-gradient(180deg, rgba(16, 12, 24, 0.96), rgba(8, 7, 12, 0.98));
      border: 1px solid rgba(255, 255, 255, 0.06);
      border-radius: 18px;
      box-shadow: 0 10px 22px rgba(0, 0, 0, 0.24);
      transform-origin: center center;
      transform: translateZ(0) scale(1);
      transition:
        transform 220ms cubic-bezier(0.2, 0.8, 0.2, 1),
        box-shadow 220ms ease,
        border-color 220ms ease;
      will-change: transform;
      z-index: 0;
    }}
    @media (hover: hover) and (pointer: fine) {{
      .wall-tile:hover {{
        transform: translate3d(0, -8px, 72px) scale(1.11);
        border-color: rgba(255, 255, 255, 0.16);
        box-shadow: 0 30px 52px rgba(0, 0, 0, 0.34);
        z-index: 3;
      }}
    }}
    .wall-meta {{
      display: flex;
      justify-content: space-between;
      gap: 10px;
      color: #dccfa9;
      font-family: "Iosevka", "JetBrains Mono", "SFMono-Regular", monospace;
      font-size: 0.84rem;
    }}
    .neuron-grid {{
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(400px, 1fr));
      gap: 18px;
    }}
    .neuron-card {{
      background: linear-gradient(180deg, rgba(30, 20, 40, 0.98), rgba(15, 11, 21, 0.98));
      border: 1px solid rgba(255, 255, 255, 0.08);
      border-radius: 24px;
      padding: 18px;
      box-shadow: 0 22px 44px rgba(0, 0, 0, 0.24);
    }}
    .neuron-header {{
      display: flex;
      justify-content: space-between;
      gap: 14px;
      align-items: flex-start;
      margin-bottom: 14px;
    }}
    .subtitle {{
      color: var(--muted-2);
      line-height: 1.45;
    }}
    .score-pill {{
      flex: 0 0 auto;
      text-align: right;
      font-family: "Iosevka", "JetBrains Mono", "SFMono-Regular", monospace;
      background: rgba(255, 255, 255, 0.05);
      border: 1px solid rgba(255, 255, 255, 0.09);
      border-radius: 16px;
      padding: 10px 12px;
      color: #ffd589;
      line-height: 1.35;
    }}
    .score-pill span {{ color: var(--muted); }}
    .neuron-meta {{
      margin-top: 13px;
      display: flex;
      flex-wrap: wrap;
      gap: 8px;
    }}
    .neuron-meta span {{
      padding: 7px 10px;
      border-radius: 999px;
      background: rgba(255, 255, 255, 0.05);
      border: 1px solid rgba(255, 255, 255, 0.07);
      color: #f0dfc4;
      font-size: 0.88rem;
    }}
    .stats-layout {{
      display: grid;
      grid-template-columns: minmax(320px, 1.15fr) minmax(280px, 0.85fr);
      gap: 18px;
      align-items: start;
    }}
    table {{
      width: 100%;
      border-collapse: collapse;
      margin-top: 10px;
      font-size: 0.96rem;
    }}
    th, td {{
      padding: 10px 8px;
      border-bottom: 1px solid rgba(255, 255, 255, 0.08);
      text-align: right;
    }}
    th:first-child, td:first-child {{ text-align: left; }}
    .bias-grid {{
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(145px, 1fr));
      gap: 10px;
      margin-top: 10px;
    }}
    .bias-tile {{
      display: grid;
      grid-template-columns: auto 22px auto;
      gap: 10px;
      align-items: center;
      padding: 12px;
      border-radius: 16px;
      background: rgba(255, 255, 255, 0.04);
      border: 1px solid rgba(255, 255, 255, 0.07);
    }}
    .bias-label {{ color: var(--muted); }}
    .bias-chip {{
      width: 22px;
      height: 22px;
      border-radius: 999px;
      box-shadow: inset 0 0 0 1px rgba(255, 255, 255, 0.14);
    }}
    .bias-value {{
      text-align: right;
      font-family: "Iosevka", "JetBrains Mono", "SFMono-Regular", monospace;
      color: #fff1ca;
    }}
    .heatmap-grid {{
      display: grid;
      grid-template-columns: 1.1fr 0.9fr;
      gap: 18px;
      align-items: start;
    }}
    .bucket-grid {{
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(260px, 1fr));
      gap: 14px;
    }}
    .heatmap-panel svg {{
      border-radius: 12px;
      background: #040308;
      margin-top: 10px;
      display: block;
    }}
    details {{
      margin-top: 16px;
      border-top: 1px solid rgba(255, 255, 255, 0.08);
      padding-top: 14px;
    }}
    summary {{
      cursor: pointer;
      color: #ffd38d;
      font-weight: 600;
      margin-bottom: 10px;
    }}
    pre {{
      margin: 0;
      white-space: pre-wrap;
      word-break: break-word;
      color: #d4c8d9;
      font-family: "Iosevka", "JetBrains Mono", "SFMono-Regular", monospace;
      line-height: 1.45;
    }}
    @media (max-width: 980px) {{
      .stats-layout,
      .heatmap-grid {{
        grid-template-columns: 1fr;
      }}
      .atlas-row:not(.atlas-row-compact) {{
        grid-template-columns: repeat(3, minmax(0, 1fr));
      }}
      .neuron-grid {{
        grid-template-columns: 1fr;
      }}
    }}
    @media (max-width: 620px) {{
      main {{ padding: 20px 14px 44px; }}
      .hero, .panel, .neuron-card {{ padding: 16px; border-radius: 18px; }}
      .atlas-row:not(.atlas-row-compact) {{ grid-template-columns: repeat(2, minmax(0, 1fr)); }}
      .wall-grid {{ grid-template-columns: 1fr; }}
      .neuron-header {{ flex-direction: column; }}
      .score-pill {{ text-align: left; }}
    }}
  </style>
</head>
<body>
  <main>
    <section class="hero">
      <div>
        <h1>Eleanor NNUE Atlas</h1>
        <p>The report leads with board-shaped NNUE views and keeps the denser neuron wall for quick scanning, so you can read Eleanor's hidden-layer structure directly from the weights.</p>
      </div>
      <div class="hero-meta">
        <span>Source <code>{html.escape(str(source_path))}</code></span>
        <span>{header_badge}</span>
        <span>{HL_SIZE} hidden neurons</span>
        <span>{OUTPUT_BUCKETS} output buckets</span>
        <span>Atlas sort <code>{html.escape(sort_mode)}</code></span>
        <span>Showing {len(top_neurons)} neurons</span>
      </div>
      <div class="legend-block">
        <div class="legend-bar"></div>
        <div class="legend-labels"><span>negative</span><span>zero</span><span>positive</span></div>
      </div>
      <details>
        <summary>Network header</summary>
        <pre>{html.escape(header_text)}</pre>
      </details>
    </section>
    <section class="section">
      <div class="section-copy">
        <h2>Feature Energy</h2>
        <p>Average absolute accumulator weight per input feature across the full hidden layer. These values are non-negative, so darker means lower feature energy and brighter means higher feature energy.</p>
      </div>
      <section class="panel">
        {energy_atlas}
        <p class="atlas-note">Board order in both rows is <code>P</code>, <code>N</code>, <code>B</code>, <code>R</code>, <code>Q</code>, <code>K</code>. This gives you the same piece-square visual language as the screenshot, but summarized across the whole network.</p>
      </section>
    </section>

    <section class="section">
      <div class="section-copy">
        <h2>Neuron Wall</h2>
        <p>A fast scan across the selected neurons using the same two-row, six-piece layout. Cool colors are negative, warm colors are positive, and black borders keep the piece boards visually separate.</p>
      </div>
      <section class="panel">
        {neuron_wall}
      </section>
    </section>

    <section class="section">
      <div class="section-copy">
        <h2>Neuron Atlas</h2>
        <p>Top hidden neurons ranked by <code>{html.escape(sort_mode)}</code>. Each card shares a zero-centered signed color scale across its twelve boards, clipped slightly so outliers do not crush the rest of the pattern.</p>
      </div>
      <div class="neuron-grid">
        {neuron_cards}
      </div>
    </section>

    <section class="section">
      <div class="section-copy">
        <h2>Tensor Overview</h2>
        <p>The atlas is the main event now, but the report still keeps a higher-level look at the full accumulator matrix and the final bucket weights for when you want the broad shape of the network.</p>
      </div>
      <div class="stats-layout">
        <section class="panel">
          <h3>Global Stats</h3>
          <table>
            <thead><tr><th>Tensor</th><th>Min</th><th>Max</th><th>Mean</th><th>Mean |x|</th></tr></thead>
            <tbody>
              {summarize_stats('Accumulator Weights (768x1536 feature-major)', acc_stats)}
              {summarize_stats('Accumulator Biases (1536)', bias_stats)}
              {summarize_stats('Output Weights (8x3072)', outw_stats)}
              {summarize_stats('Output Biases (8)', outb_stats)}
            </tbody>
          </table>
        </section>
        {make_bucket_biases(data.out_biases)}
      </div>
    </section>

    <section class="section">
      <div class="heatmap-grid">
        {make_svg_heatmap(acc_pool, pool_rows, pool_cols, 'Accumulator Matrix', f'features x neurons = {INPUT_SIZE}x{HL_SIZE}, pooled to {pool_rows}x{pool_cols}', pixel=5, clip_quantiles=(0.02, 0.98), signed_scale=True)}
        <section class="panel">
          <h3>Output Bucket Maps</h3>
          <p class="subtitle">Each mini-heatmap shows the two output rows for one bucket: side-to-move accumulator on top, non-side-to-move on bottom.</p>
          <div class="bucket-grid">
            {''.join(bucket_panels)}
          </div>
        </section>
      </div>
    </section>
  </main>
</body>
</html>
"""


def main() -> None:
    args = parse_args()
    pool_cols, pool_rows = parse_pool(args.pool)

    data = load_nnue(args.nnue)
    report = build_html(
        data,
        pool_cols=pool_cols,
        pool_rows=pool_rows,
        bucket_cols=args.bucket_cols,
        neuron_count=args.neurons,
        sort_mode=args.sort,
        source_path=args.nnue,
    )

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(report, encoding="utf-8")
    print(f"Wrote NNUE visualization: {args.output}")


if __name__ == "__main__":
    main()
