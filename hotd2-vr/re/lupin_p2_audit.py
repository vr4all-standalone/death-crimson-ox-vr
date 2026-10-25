"""Offline numeric P2 prompt audit; never launches a game or writes game payloads.
Requires numpy and the existing assets/riplib.py. Example:
  python re/lupin_p2_audit.py --capture-root <private-vrtest-root> --output <ignored-json>
"""
import sys,struct,json,math,argparse
from pathlib import Path
repo=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(repo/'hotd2-vr/assets'))
from riplib import Pass
import numpy as np
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--capture-root',action='append',type=Path,help='Root containing <run>/rip/passNNNN.bin; repeat as needed')
parser.add_argument('--output',type=Path,default=repo/'build-win/vrtest/lupin-p2-evidence.json')
args=parser.parse_args()
roots=args.capture_root or [repo/'build-win/vrtest']
records=[]
for root in roots:
 for path in sorted(root.glob('*/rip/pass????.bin')):
  try:
   n=int(path.stem[4:]);ps=Pass(str(path.parent),n)
   with open(ps._latest('ram'),'rb') as f:
    vals=[]
    for addr in (0xa4c728,0xa4c738,0xa4c73c,0xa4c748,0xa4c74c):
     f.seek(addr);vals.append(struct.unpack('<f',f.read(4))[0])
   if not all(math.isfinite(x) for x in vals):continue
   hit=[]
   for poly in ps.polys:
    v=poly.v
    if poly.tex!=0x92d800 or len(v)<3:continue
    with np.errstate(divide='ignore',invalid='ignore'): w=1/v['z']
    if not np.isfinite(w).all() or not np.isfinite(v['x']).all() or not np.isfinite(v['y']).all():continue
    box=[float(v['x'].min()),float(v['y'].min()),float(v['x'].max()),float(v['y'].max())]
    if not (w.min()>=.49 and w.max()<=.75 and box[0]>=437.5 and box[1]>=440.5 and box[2]<=617.5 and box[3]<=464.5):continue
    hit.append(dict(lst=poly.lst,index=poly.idx,box=box,width=box[2]-box[0],height=box[3]-box[1],glyph=(box[2]-box[0]>=8 and box[3]-box[1]>=8),w=float(w.mean()),uv=[float(v['u'].min()),float(v['v'].min()),float(v['u'].max()),float(v['v'].max())]))
   records.append(dict(path=str(path),session=vals[0],players=[vals[1],vals[2]],life=[vals[3],vals[4]],eligible=vals[:3]==[1,1,0],count=len(hit),glyphcount=sum(h['glyph'] for h in hit),hits=hit))
  except (FileNotFoundError,ValueError,OSError):pass
out=args.output
out.parent.mkdir(parents=True,exist_ok=True)
out.write_text(json.dumps(records,indent=2))
from collections import Counter
print('total passes',len(records),'eligible',sum(r['eligible'] for r in records))
print('eligible counts',Counter(r['count'] for r in records if r['eligible']))
print('eligible glyph counts (min 8x8)',Counter(r['glyphcount'] for r in records if r['eligible']))
for kind,pred in [('eligible prompt',lambda r:r['eligible'] and r['count']==6),('eligible absence',lambda r:r['eligible'] and r['count']==0),('eligible other',lambda r:r['eligible'] and r['count'] not in (0,6)),('active P2 collisions',lambda r:r['session']==1 and r['players'][1]==1 and r['count']>0),('P1 active P2 continue',lambda r:r['session']==1 and r['players'][0]==1 and r['life'][1]==2)]:
 rows=[r for r in records if pred(r)];print('\n'+kind,len(rows))
 for r in rows[:12]:print(r['path'],'players',r['players'],'life',r['life'],'count',r['count'])
glyphs=[h for r in records if r['eligible'] for h in r['hits'] if h['glyph']]
print('eligible glyph sizes',Counter((h['width'],h['height']) for h in glyphs))
print('active P2 min8 collisions',sum(r['session']==1 and r['players'][1]==1 and r['glyphcount']>0 for r in records))
print('numeric evidence',out)
