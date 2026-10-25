"""Check actual game VM aim after the production calibration inverse (local RAM only)."""
import argparse
import json
import struct
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('run', type=Path)
p.add_argument('--focal', type=float, required=True)
a = p.parse_args()
passes = list((a.run / 'rip').glob('pass*.bin'))
assert passes, 'no actual render passes'
for f in passes:
    magic, w, h, fx, fy, hud, stock, lists = struct.unpack_from('<4sIIffffI', f.read_bytes())
    assert magic == b'RIP1' and abs(stock - 879.4259033203125) < .01, (f, stock)
    assert abs(fx - a.focal) < .01 and abs(fy - a.focal) < .01, (f, fx, fy)
print('PASS live rendering focal and independent stock hit-test focal on every pass')
shots = [[], []]
for pl in range(2):
    for f in sorted((a.run / 'rip').glob('ram*.bin')):
        b = f.read_bytes()
        def value(off): return struct.unpack_from('<f', b, off)[0]
        assert list(b[0xb67da4:0xb67db0]) == [13,228,12,238,122,124,20,240,8,232,130,118], f
        assert abs(value(0xbd6120) - a.focal) < .01, f
        shots[pl].append(dict(file=f.name, n=value(0xa4c828+4*pl),
                              xy=[value(0xa4c788+4*pl), value(0xa4c798+4*pl)]))
points = [[[128,96],[320,96],[512,96],[128,240],[320,240],[512,240],
           [128,384],[320,384],[512,384],[0,0],[639,479]],
          [[128,96],[320,240],[512,384],[0,0],[639,479]]]
report = []
for pl in range(2):
    for n, target in enumerate(points[pl], 1):
        matches = [s for s in shots[pl] if s['n'] == n and max(abs(x-y) for x,y in zip(s['xy'],target)) <= 2]
        assert matches, (pl+1,n,target,'no actual VM point within two pixels')
        report.append(dict(player=pl+1,shot=n,target=target,measured=matches[0]['xy'],proof=matches[0]['file']))
        print('PASS P%d shot%d target%s actual%s' % (pl+1,n,target,matches[0]['xy']))
(a.run / 'aim-evidence.json').write_text(json.dumps(report, indent=2))
print('PASS 16 actual game points, both players, unchanged calibration and expected focal')
