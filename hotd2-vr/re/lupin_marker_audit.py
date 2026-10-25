#!/usr/bin/env python3
"""Read-only numeric audit of original GDS-0018 Flycast RIP1 render passes.

Uses only Python's standard library. Does not read RAM, VRAM, palettes, ROMs,
textures or images. See ../notes/lupin-hit-marker-verification.md.
Copyright 2026. Part of the Lupin VR fork of Flycast, GPL v2 or later.
"""
from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
import struct
import sys
import unittest

HEADER = struct.Struct('<4sIIffffI')
POLYGON = struct.Struct('<5I')
VERTEX = struct.Struct('<5f4B4B')
MARKER_TEXTURE = 0x92D800
MARKER_W = (0.52630, 0.52633)
UV_BOXES = ((0.25, 0.0, 0.5, 0.375),
            (0.5, 0.5, 0.5625, 0.5625),
            (0.875, 0.375, 1.0, 0.5))
MAX_PASS_BYTES = 128 * 1024 * 1024


def read_pass(data: bytes) -> dict:
    """Bounds-check RIP1 before decoding; allocation is bounded by file size."""
    if not HEADER.size <= len(data) <= MAX_PASS_BYTES:
        raise ValueError('invalid RIP1 file size')
    magic, width, height, fx, fy, hud_w, stock, lists = HEADER.unpack_from(data)
    if magic != b'RIP1' or not 1 <= width <= 16384 or not 1 <= height <= 16384:
        raise ValueError('invalid RIP1 magic or dimensions')
    if not 1 <= lists <= 3 or not all(map(math.isfinite, (fx, fy, hud_w, stock))):
        raise ValueError('invalid RIP1 lists or lens metadata')
    offset = HEADER.size
    polygons = []
    for lst in range(lists):
        if offset + 4 > len(data):
            raise ValueError('truncated RIP1 list')
        count, = struct.unpack_from('<I', data, offset)
        offset += 4
        if count > (len(data) - offset) // POLYGON.size:
            raise ValueError('RIP1 polygon count exceeds remaining data')
        for index in range(count):
            if offset + POLYGON.size > len(data):
                raise ValueError('truncated RIP1 polygon')
            isp, tsp, tcw, pcw, nv = POLYGON.unpack_from(data, offset)
            offset += POLYGON.size
            if nv > (len(data) - offset) // VERTEX.size:
                raise ValueError('RIP1 vertex count exceeds remaining data')
            vertices = [VERTEX.unpack_from(data, offset + i * VERTEX.size) for i in range(nv)]
            offset += nv * VERTEX.size
            polygons.append(dict(list=lst, index=index, isp=isp, tsp=tsp,
                                 tcw=tcw, pcw=pcw, vertices=vertices))
    if offset != len(data):
        raise ValueError('unexpected trailing RIP1 data')
    return dict(width=width, height=height, focal=[fx, fy], hud_w=hud_w,
                stock_focal=stock, polygons=polygons)


def describe(poly: dict, width: int, height: int, hud_w: float) -> dict:
    """Classify original geometry, BEFORE UI depth snapping.

    Marker candidates are geometry evidence only: runtime suppression also needs
    recent accepted 3D-shot/input proximity and the original-revision code guards.
    A flash candidate is deliberately broad; vertex colour alone does not prove
    the textured output is white, nor that a fade was caused by a gun shot.
    """
    result = dict(valid_geometry=False, marker_geometry_candidate=False,
                  shared_atlas_depth_other=False, ui_flash_candidate=False,
                  bright_full_screen_world=False)
    vertices = poly['vertices']
    if len(vertices) < 3 or any(v[2] <= 0 or not all(map(math.isfinite, v[:5])) for v in vertices):
        return result
    xs, ys, zs, us, vs = (list(values) for values in zip(*(v[:5] for v in vertices)))
    ws = [1 / z for z in zs]
    bounds = (min(xs), min(ys), max(xs), max(ys))
    uv = (min(us), min(vs), max(us), max(vs))
    texture = (poly['tcw'] & 0x1FFFFF) << 3 if poly['pcw'] & 8 else 0
    in_band = min(ws) >= MARKER_W[0] and max(ws) <= MARKER_W[1]
    in_region = any(uv[0] >= a and uv[1] >= b and uv[2] <= c and uv[3] <= d
                    for a, b, c, d in UV_BOXES)
    marker = (texture == MARKER_TEXTURE and poly['list'] == 2 and len(vertices) == 4
              and in_band and in_region and uv[0] < uv[2] and uv[1] < uv[3])
    bright = all(c >= 0xC0 for c in vertices[0][5:8])
    full_screen = bounds[2] - bounds[0] >= width * 0.9 and bounds[3] - bounds[1] >= height * 0.9
    # Cover all measured original LP UI layers as well as an already-snapped
    # header overlay. This is a superset of the runtime onOverlay test.
    ui_depth = (min(ws) >= 0.49 and max(ws) <= 0.75) or (
        max(ws) <= 1.15 and all(abs(w - hud_w) < 0.002 for w in ws))
    result.update(valid_geometry=True, marker_geometry_candidate=marker,
                  shared_atlas_depth_other=texture == MARKER_TEXTURE and in_band and not marker,
                  ui_flash_candidate=bool(bright and full_screen and ui_depth),
                  bright_full_screen_world=bool(bright and full_screen and min(ws) > 1.15),
                  texture=f'0x{texture:06X}', list=poly['list'], index=poly['index'],
                  vertex_count=len(vertices), bounds=list(bounds),
                  centre=[(bounds[0] + bounds[2]) / 2, (bounds[1] + bounds[3]) / 2],
                  w_range=[min(ws), max(ws)], uv=list(uv), first_rgba=list(vertices[0][5:9]))
    return result


COUNT_KEYS = ('marker_geometry_candidate', 'shared_atlas_depth_other',
              'ui_flash_candidate', 'bright_full_screen_world')


def audit_paths(paths):
    files = set()
    for path in paths:
        path = Path(path)
        if path.is_dir():
            matches = list(path.glob('pass[0-9][0-9][0-9][0-9].bin'))
            if not matches:
                raise ValueError(f'no RIP1 pass files in {path}')
            files.update(p.resolve() for p in matches)
        elif path.is_file():
            files.add(path.resolve())
        else:
            raise ValueError(f'input does not exist: {path}')
    report = dict(format='lupin-marker-audit-v1', revision='original GDS-0018 only',
                  note='Geometry candidates only; no shot-time correlation or texture decoding.',
                  summary=dict(passes=0, polygons=0, invalid_geometry=0,
                               **dict.fromkeys(COUNT_KEYS, 0)),
                  marker_uv_rectangles=[], shared_hud_uv_rectangles=[], passes=[])
    marker_uv, other_uv = set(), set()
    for path in sorted(files):
        # Read at most the configured bound even if the input is a growing file.
        with path.open('rb') as source:
            raw = source.read(MAX_PASS_BYTES + 1)
        try:
            capture = read_pass(raw)
        except ValueError as error:
            raise ValueError(f'{path}: {error}') from error
        entry = dict(file=str(path), width=capture['width'], height=capture['height'],
                     hud_w=capture['hud_w'], focal=capture['focal'],
                     stock_focal=capture['stock_focal'], polygons=len(capture['polygons']),
                     invalid_geometry=0, counts=dict.fromkeys(COUNT_KEYS, 0), candidates=[])
        for poly in capture['polygons']:
            item = describe(poly, capture['width'], capture['height'], capture['hud_w'])
            entry['invalid_geometry'] += not item['valid_geometry']
            for key in COUNT_KEYS:
                entry['counts'][key] += item[key]
                report['summary'][key] += item[key]
            if item['marker_geometry_candidate']:
                marker_uv.add(tuple(round(x, 9) for x in item['uv']))
            elif item['shared_atlas_depth_other']:
                other_uv.add(tuple(round(x, 9) for x in item['uv']))
            if any(item[key] for key in COUNT_KEYS):
                entry['candidates'].append(item)
        report['summary']['passes'] += 1
        report['summary']['polygons'] += entry['polygons']
        report['summary']['invalid_geometry'] += entry['invalid_geometry']
        report['passes'].append(entry)
    report['marker_uv_rectangles'] = sorted(marker_uv)
    report['shared_hud_uv_rectangles'] = sorted(other_uv)
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('inputs', nargs='+', type=Path, help='RIP1 pass files or rip directories')
    parser.add_argument('--output', type=Path, help='write numeric JSON to a NEW file; otherwise stdout')
    args = parser.parse_args()
    try:
        report = audit_paths(args.inputs)
        if args.output:
            # Never overwrite captures, ROMs or a prior report.
            with args.output.open('x', encoding='utf-8') as target:
                json.dump(report, target, indent=2, allow_nan=False)
                target.write('\n')
            print(json.dumps(report['summary'], sort_keys=True))
        else:
            print(json.dumps(report, indent=2, allow_nan=False))
    except (OSError, ValueError) as error:
        parser.exit(2, f'{parser.prog}: {error}\n')


def fixture(*, uv=(0.3, 0.1, 0.4, 0.2), w=1 / 1.9, texture=MARKER_TEXTURE,
            lst=2, rect=(100, 100, 120, 120), rgb=(255, 255, 255), n=4):
    x0, y0, x1, y1 = rect
    u0, v0, u1, v1 = uv
    vertices = [(x0, y0, 1 / w, u0, v0, *rgb, 255, 0, 0, 0, 0),
                (x1, y0, 1 / w, u1, v0, *rgb, 255, 0, 0, 0, 0),
                (x0, y1, 1 / w, u0, v1, *rgb, 255, 0, 0, 0, 0),
                (x1, y1, 1 / w, u1, v1, *rgb, 255, 0, 0, 0, 0)]
    return {'list': lst, 'index': 0, 'isp': 0, 'tsp': 0,
            'tcw': texture >> 3, 'pcw': 8 if texture else 0,
            'vertices': vertices[:n]}


def encode(poly):
    data = HEADER.pack(b'RIP1', 640, 480, 381.4, 381.4, 0.6, 879.4, 3)
    for lst in range(3):
        data += struct.pack('<I', int(poly['list'] == lst))
        if poly['list'] == lst:
            data += POLYGON.pack(poly['isp'], poly['tsp'], poly['tcw'], poly['pcw'],
                                 len(poly['vertices']))
            data += b''.join(VERTEX.pack(*v) for v in poly['vertices'])
    return data


class AuditTests(unittest.TestCase):
    def classify(self, **kwargs):
        return describe(fixture(**kwargs), 640, 480, 0.6)

    def test_three_marker_regions_and_clipped_subset(self):
        for uv in (*UV_BOXES, (0.2822, 0.2822, 0.2881, 0.2862)):
            with self.subTest(uv=uv):
                self.assertTrue(self.classify(uv=uv)['marker_geometry_candidate'])

    def test_shared_hud_glyph_is_negative(self):
        result = self.classify(uv=(0.01, 0.75, 0.03, 0.8))
        self.assertFalse(result['marker_geometry_candidate'])
        self.assertTrue(result['shared_atlas_depth_other'])

    def test_region_crossing_is_negative(self):
        self.assertFalse(self.classify(uv=(0.49, 0.1, 0.51, 0.2))['marker_geometry_candidate'])

    def test_opaque_and_punch_through_are_negative(self):
        for lst in (0, 1):
            self.assertFalse(self.classify(lst=lst)['marker_geometry_candidate'])

    def test_wrong_texture_and_untextured_are_negative(self):
        for texture in (0, 0xC65800):
            self.assertFalse(self.classify(texture=texture)['marker_geometry_candidate'])

    def test_world_effect_and_snapped_marker_are_negative(self):
        for w in (27.22, 0.598995):
            self.assertFalse(self.classify(w=w)['marker_geometry_candidate'])

    def test_non_quad_is_negative(self):
        self.assertFalse(self.classify(n=3)['marker_geometry_candidate'])

    def test_variable_depth_is_negative(self):
        poly = fixture()
        poly['vertices'][0] = (*poly['vertices'][0][:2], 2.0, *poly['vertices'][0][3:])
        self.assertFalse(describe(poly, 640, 480, 0.6)['marker_geometry_candidate'])

    def test_nonfinite_geometry_is_invalid(self):
        poly = fixture()
        poly['vertices'][0] = (math.nan, *poly['vertices'][0][1:])
        self.assertFalse(describe(poly, 640, 480, 0.6)['valid_geometry'])

    def test_fullscreen_ui_flash_candidate(self):
        self.assertTrue(self.classify(w=0.6, rect=(0, 0, 640, 480))['ui_flash_candidate'])
        self.assertTrue(self.classify(w=0.5263158, rect=(0, 0, 640, 480))['ui_flash_candidate'])

    def test_fullscreen_world_effect_is_separate(self):
        result = self.classify(w=27.22, rect=(-50, -50, 700, 550))
        self.assertFalse(result['ui_flash_candidate'])
        self.assertTrue(result['bright_full_screen_world'])

    def test_dark_or_small_ui_quad_is_not_flash(self):
        self.assertFalse(self.classify(w=0.6, rgb=(191, 255, 255), rect=(0, 0, 640, 480))['ui_flash_candidate'])
        self.assertFalse(self.classify(w=0.6)['ui_flash_candidate'])

    def test_round_trip_rip_metadata(self):
        result = read_pass(encode(fixture()))
        self.assertEqual((result['width'], result['height']), (640, 480))
        self.assertEqual(len(result['polygons']), 1)
        self.assertTrue(describe(result['polygons'][0], 640, 480, result['hud_w'])['marker_geometry_candidate'])

    def test_truncation_at_every_byte(self):
        data = encode(fixture())
        for length in range(len(data)):
            with self.subTest(length=length):
                with self.assertRaises(ValueError):
                    read_pass(data[:length])

    def test_bad_magic_counts_dimensions_and_trailing_data(self):
        good = encode(fixture())
        for data in (b'BAD!' + good[4:], good[:28] + struct.pack('<I', 0xFFFFFFFF) + good[32:],
                     good[:4] + struct.pack('<I', 0) + good[8:], good + b'extra',
                     good[:32] + struct.pack('<I', 0xFFFFFFFF) + good[36:]):
            with self.subTest(header=data[:36]):
                with self.assertRaises(ValueError):
                    read_pass(data)


if __name__ == '__main__':
    if sys.argv[1:] == ['--self-test']:
        unittest.main(argv=[sys.argv[0]], verbosity=2)
    else:
        main()
