// Death Crimson OX native state and calibration regression checks. GPL-2.0-or-later.
#include "../../core/rend/vr/death_gun.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unordered_map>
#include <limits>
#include <utility>
using namespace vr::death;
static int failures;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); ++failures; } } while(0)
// An independent cabinet-path oracle: raw u16 JVS conversion followed by SH4 float arithmetic.
static int cabinet(int p,int extent,int lo,int hi) {
 const int raw=p*65535/(extent-1);
 if(raw<lo || raw>hi) return -1;
 const float delta=float(raw-lo), span=float(hi-lo);
 return int((delta*float(extent))/span);
}
int main() {
 Calibration c{{11264,60416},{13056,64256}};
 CHECK(nativeAxis(320,640,c.x)==280);
 CHECK(nativeAxis(240,480,c.y)==185);
 CHECK(nativeAxis(0,640,c.x)==-1);
 CalibrationMap map;
 int x=320,y=240;
 CHECK(calibrateInput(c,true,map,x,y));
 CHECK(std::abs(cabinet(x,640,11264,60416)-320)<=1);
 CHECK(std::abs(cabinet(y,480,13056,64256)-240)<=1);
 for(int p=0;p<640;++p) { int px=p,py=p%480; CHECK(calibrateInput(c,true,map,px,py)); CHECK(std::abs(cabinet(px,640,11264,60416)-p)<=1); CHECK(std::abs(cabinet(py,480,13056,64256)-p%480)<=1); }
 for(auto xy : {std::pair<int,int>{-10000,-10000},{-1,240},{640,240},{320,480}}) { x=xy.first;y=xy.second; CHECK(!calibrateInput(c,true,map,x,y)); CHECK(x==xy.first && y==xy.second); }
 x=320;y=240; CHECK(!calibrateInput(c,false,map,x,y)); CHECK(x==320 && y==240);
 Calibration bad{{65535,0},{13056,64256}}; CHECK(!calibrateInput(bad,true,map,x,y)); CHECK(x==320 && y==240);
 Calibration different{{0,65535},{0,65535}}; CHECK(calibrateInput(different,true,map,x,y)); CHECK(std::abs(cabinet(x,640,0,65535)-320)<=1);
 std::unordered_map<unsigned,unsigned> ram;
 for(const auto& g: CodeGuards) ram[g.offset]=g.word;
 auto r8=[&](unsigned a){return unsigned(ram[a]&255);};
 auto r16=[&](unsigned a){CHECK((a&1)==0 && a<=0x1FFFFFE);return unsigned(ram[a]&65535);};
 auto r32=[&](unsigned a){CHECK((a&3)==0 && a<=0x1FFFFFC);return unsigned(ram[a]);};
 ram[0x18BFC9]=9;ram[0x18BFD5]=1;ram[0x097AE0]=0x12345;ram[0x0974AC]=0x40400000;
 ram[0x18C005]=6;ram[0x18C011]=1;ram[0x097AE4]=7;ram[0x0974B0]=0x40000000;
 auto a=readGun(0,r8,r16,r32); CHECK(a.known && a.active && a.ammo==9 && a.magazine==10 && a.shots==0x12345);
 auto b=readGun(1,r8,r16,r32); CHECK(b.known && b.active && b.ammo==6 && b.shots==7);
 ram[0x18BFD5]=0; a=readGun(0,r8,r16,r32); CHECK(a.known && !a.active);
 ram[0x18BFD5]=1;ram[0x0974AC]=0; a=readGun(0,r8,r16,r32); CHECK(a.known && !a.active);
 ram[0x0974AC]=0x7fc00000; CHECK(!readGun(0,r8,r16,r32).known);
 ram[0x0974AC]=0x40400000;ram[0x18BFC9]=255; CHECK(!readGun(0,r8,r16,r32).known);
 ram[0x18BFC9]=10;ram[0x18BFD5]=7; CHECK(!readGun(0,r8,r16,r32).known);
 ram[0x18BFD5]=1;ram[CodeGuards[0].offset]^=1; CHECK(!readGun(0,r8,r16,r32).known); ram[CodeGuards[0].offset]^=1;
 CHECK(!readGun(-1,r8,r16,r32).known); CHECK(!readGun(2,r8,r16,r32).known);
 ram[0x097A68]=11264;ram[0x097A70]=60416;ram[0x097AB0]=13056;ram[0x097AB8]=64256;
 ram[0x097A8C]=12288;ram[0x097A94]=61440;ram[0x097AD4]=14080;ram[0x097ADC]=65280;
 Calibration got;CHECK(readCalibration(0,r16,r32,got));CHECK(got.x.minimum==11264&&got.x.maximum==60416&&got.y.minimum==13056);
 CHECK(readCalibration(1,r16,r32,got));CHECK(got.x.minimum==12288&&got.y.maximum==65280);
 CHECK(!readCalibration(2,r16,r32,got));ram[0x097A68]=70000;CHECK(!readCalibration(0,r16,r32,got));
 // Mode 2 is native MOVIE; mode 0 is interactive SHOOTING. Fixture mirrors the
 // captured intro's root -> scene table -> descriptor -> 64-byte frame chain.
 for(const auto& g: SceneGuards) ram[g.offset]=g.word;
 ram[0x09A23C]=0x8C18BF24;ram[0x18BFAC]=1;
 ram[0x18BF24]=0;ram[0x18BF26]=6;ram[0x18BF28]=1;ram[0x18BF2A]=9;
 ram[0x18BF48]=0x8CB69320;ram[0xB69388]=0x8CB692E0;
 ram[0xB692F4]=0x8CB68DC0;ram[0xB68E02]=2;
 auto scene=readScene(r16,r32);CHECK(scene.known && scene.movie && scene.mode==2);
 ram[0xB68E02]=0;scene=readScene(r16,r32);CHECK(scene.known && !scene.movie && scene.mode==0);
 ram[0x18BF24]=2;scene=readScene(r16,r32);CHECK(scene.known && scene.movie);
 ram[0x18BF24]=4;scene=readScene(r16,r32);CHECK(scene.known && !scene.movie);
 ram[0x18BF24]=0;ram[0x18BF26]=65535;scene=readScene(r16,r32);CHECK(scene.known && !scene.movie && scene.mode==0);
 ram[0x18BF26]=65534;CHECK(!readScene(r16,r32).known);
 ram[0x18BF26]=512;CHECK(!readScene(r16,r32).known);
 ram[0x18BF26]=6;ram[0x18BF28]=65535;CHECK(!readScene(r16,r32).known);
 ram[0x18BF28]=4096;CHECK(!readScene(r16,r32).known);
 ram[0x18BF28]=1;ram[0x18BF2A]=0;scene=readScene(r16,r32);CHECK(scene.known && !scene.movie && scene.mode==0);
 ram[0x18BF2A]=9;
 for(unsigned* field : {&ram[0x09A23C],&ram[0x18BF48],&ram[0xB69388],&ram[0xB692F4]}) {
  const unsigned saved=*field;
  for(unsigned badPointer : {0u,0x8C000001u,0x8DFFFFFCu,0x8E000000u,0xFFFFFFFFu}) {
   *field=badPointer;scene=readScene(r16,r32);CHECK(!scene.known && !scene.movie);
  }
  *field=saved;
 }
 ram[0x18BFAC]=0;scene=readScene(r16,r32);CHECK(!scene.known && !scene.movie);ram[0x18BFAC]=1;
 ram[SceneGuards[0].offset]^=1;CHECK(!readScene(r16,r32).known);ram[SceneGuards[0].offset]^=1; std::printf("Death gun: %d failures\n",failures);return failures?1:0;
}