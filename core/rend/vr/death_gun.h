// Death Crimson OX (Japan Rev A) native gun data. GPL-2.0-or-later.
#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <limits>
namespace vr { namespace death {
// Offsets are in the 32 MB NAOMI RAM, not host or SH4 virtual addresses.
// The decrement, accepted-shot increment and u16 ADC parser must all match.
struct Guard { unsigned offset,word; };
inline constexpr Guard CodeGuards[] = {
 {0x036E08,0x84E1},{0x036E14,0x70FF},{0x036E16,0x80E1},
 {0x03D708,0xD146},{0x03D718,0x2322},
 {0x07407A,0x600D},{0x074090,0xF233},{0x0740C0,0x81E3},
 {0x0369D8,0xBFC8},{0x0369DA,0x8C18}, // player blocks
 {0x03D824,0x7AE0},{0x03D826,0x8C09}, // shot counters
 {0x073F38,0x7A50},{0x073F3A,0x8C09}, // X calibration owner
 {0x073F40,0x7A98},{0x073F42,0x8C09}  // Y calibration owner
};
template<class Read16> bool codeValid(Read16 read16) {
 for(const Guard& g:CodeGuards) if(read16(g.offset)!=g.word) return false;
 return true;
}
struct Axis { int minimum=0,maximum=0; };
struct Calibration { Axis x,y; };
inline bool validAxis(const Axis& a) {
 return a.minimum>=0 && a.maximum<=65535 && a.minimum<a.maximum;
}
// JVS uses the full unsigned word. Native code truncates the single precision
// result (raw-minimum)*extent/(maximum-minimum); its saved centre is not used.
inline int nativeAxis(int pixel,int extent,const Axis& axis) {
 if(extent<2 || extent>640 || pixel<0 || pixel>=extent || !validAxis(axis)) return -1;
 const int raw=pixel*65535/(extent-1);
 if(raw<axis.minimum || raw>axis.maximum) return -1;
 const float scaled=float(raw-axis.minimum)*float(extent);
 return int(scaled/float(axis.maximum-axis.minimum));
}
struct CalibrationMap {
 Calibration values;
 std::array<int,640> x{};
 std::array<int,480> y{};
 bool ready=false;
};
inline bool sameAxis(const Axis& a,const Axis& b) {
 return a.minimum==b.minimum && a.maximum==b.maximum;
}
// Build once per live calibration change. Reject a calibration whose available
// input pixels cannot represent every on-screen target to within one pixel.
inline bool buildAxisMap(int extent,const Axis& axis,int* out) {
 if(!out || extent<2 || extent>640 || !validAxis(axis)) return false;
 int projected[640];
 for(int p=0;p<extent;++p) projected[p]=nativeAxis(p,extent,axis);
 for(int target=0;target<extent;++target) {
  int best=-1,error=std::numeric_limits<int>::max(),displacement=error;
  for(int p=0;p<extent;++p) {
   if(projected[p]<0 || projected[p]>=extent) continue;
   const int e=std::abs(projected[p]-target),d=std::abs(p-target);
   if(e<error || (e==error && d<displacement)) { best=p;error=e;displacement=d; }
  }
  if(best<0 || error>1) return false;
  out[target]=best;
 }
 return true;
}
inline bool calibrateInput(const Calibration& calibration,bool active,CalibrationMap& map,int& x,int& y) {
 if(!active || x<0 || x>=640 || y<0 || y>=480) return false;
 if(!map.ready || !sameAxis(map.values.x,calibration.x) || !sameAxis(map.values.y,calibration.y)) {
  map.ready=false;
  map.values=calibration;
  if(!buildAxisMap(640,calibration.x,map.x.data()) || !buildAxisMap(480,calibration.y,map.y.data())) return false;
  map.ready=true;
 }
 x=map.x[x]; y=map.y[y];
 return true;
}
struct Gun {
 bool known=false,active=false;
 std::uint8_t ammo=0,magazine=10;
 std::uint32_t shots=0;
};
template<class Read8,class Read16,class Read32> Gun readGun(int player,Read8 read8,Read16 read16,Read32 read32) {
 Gun gun;
 if(player<0 || player>1 || !codeValid(read16)) return gun;
 const unsigned base=0x18BFC8+unsigned(player)*60;
 const unsigned ammo=read8(base+1),active=read8(base+13);
 const std::uint32_t bits=read32(0x0974AC+unsigned(player)*4);
 float health;
 std::memcpy(&health,&bits,sizeof(health));
 if(ammo>10 || active>1 || !std::isfinite(health) || health<0.f || health>1000.f) return gun;
 gun.known=true;
 gun.active=active==1 && health>0.f;
 gun.ammo=std::uint8_t(ammo);
 gun.shots=read32(0x097AE0+unsigned(player)*4);
 return gun;
}
template<class Read16,class Read32> bool readCalibration(int player,Read16 read16,Read32 read32,Calibration& out) {
 if(player<0 || player>1 || !codeValid(read16)) return false;
 const unsigned step=unsigned(player)*36;
 const unsigned xmin=read32(0x097A68+step),xmax=read32(0x097A70+step);
 const unsigned ymin=read32(0x097AB0+step),ymax=read32(0x097AB8+step);
 if(xmin>65535 || xmax>65535 || ymin>65535 || ymax>65535) return false;
 Calibration calibration{{int(xmin),int(xmax)},{int(ymin),int(ymax)}};
 if(!validAxis(calibration.x) || !validAxis(calibration.y)) return false;
 out=calibration;
 return true;
}
struct Scene { bool known=false,movie=false; unsigned mode=0; };
inline constexpr Guard SceneGuards[]={{0x0550B0,0x9088},{0x0550D2,0x8541},{0x0550DE,0x5349},{0x0550F4,0x8501},{0x0551C4,0x0088},{0x063F24,0xA23C},{0x063F26,0x8C09},{0x069D84,0x8802}};
// Native scene mode resolver 0x0550A2. Mode 2 is named MOVIE / MOVIE NO SKIP
// by the game's own debug view (0x069D82); 0 is interactive SHOOTING.
// The data lives in dynamically loaded scene tables, not one fixed chapter.
inline bool ramSpan(std::uint32_t pointer,unsigned bytes,unsigned& offset) {
 if((pointer&0xFE000000u)!=0x8C000000u || (pointer&3u)!=0) return false;
 offset=pointer&0x01FFFFFFu;
 return bytes<=0x02000000u-offset;
}
template<class Read16,class Read32> Scene readScene(Read16 read16,Read32 read32) {
 Scene scene;
 if(!codeValid(read16)) return scene;
 for(const Guard& g:SceneGuards) if(read16(g.offset)!=g.word) return scene;
 unsigned root;
 if(!ramSpan(read32(0x09A23C),140,root) || read32(root+136)==0) return scene;
 const int rootMode=int(std::int16_t(read16(root)));
 if(rootMode<0 || rootMode>4) return scene;
 if(rootMode==1 || rootMode==2 || rootMode==4) return {true,rootMode==2,unsigned(rootMode)};
 const int index=int(std::int16_t(read16(root+2)));
 if(index==-1) return {true,false,0};
 // Native code has already validated the live indices. No independent native
 // scene/frame count has been established, so apply conservative signed-index
 // ceilings and range-check each complete table prefix before dereferencing it.
 // root+6 is a changing frame timer, not a scene count.
 const int frame=int(std::int16_t(read16(root+4)));
 if(index<0 || index>=512 || frame<0 || frame>=4096) return scene;
 unsigned table,descriptor,frames;
 if(!ramSpan(read32(root+36),unsigned(index+1)*16,table)) return scene;
 if(!ramSpan(read32(table+unsigned(index)*16+8),24,descriptor)) return scene;
 if(!ramSpan(read32(descriptor+20),unsigned(frame+1)*64,frames)) return scene;
 const unsigned mode=read16(frames+unsigned(frame)*64+2);
 if(mode>4) return scene;
 return {true,mode==2,mode};
}}}