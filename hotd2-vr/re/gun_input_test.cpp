#include "gun_input.h"
#include "gun_calibration.h"
#include <cstdio>
int main() {
 int failed=0;
 auto check=[&](bool pass,const char* name){std::printf("%s %s\n",pass?"PASS":"FAIL",name);failed+=!pass;};
 vr::GunInput p1,p2; int x=2500,y=4000; uint32_t b=0;
 vr::adaptGunInput(p1,true,x,y,b);
 x=y=-10000;vr::adaptGunInput(p1,true,x,y,b);check(x==2500&&y==4000&&b==0,"aim out alone holds ADC; shoot-off-screen stays distinct");
 b=2;vr::adaptGunInput(p1,true,x,y,b);check(x<0&&y<0&&(b&2),"explicit point-mode reload reaches sensor");
 x=2500;y=4000;b=128;vr::adaptGunInput(p1,true,x,y,b);check(x<0&&y<0&&(b&2)&&!(b&129),"B reloads through sensor without firing");
 x=y=-10000;b=0;vr::adaptGunInput(p1,true,x,y,b);check(x==2500&&y==4000&&b==0,"reload release restores ADC");
 x=y=-10000;b=0;vr::adaptGunInput(p2,true,x,y,b);check(x==5000&&y==5000,"players have independent held ADC");
 x=y=-10000;b=1;vr::adaptGunInput(p1,true,x,y,b);check(x<0&&y<0&&(b&2)&&!(b&1),"off-screen trigger reloads without a fired round");
 x=10000;y=5000;b=0;vr::adaptGunInput(p1,true,x,y,b);check(x==2500&&y==4000&&b==0,"right endpoint is outside the 640-pixel ADC");
 x=3500;y=4500;b=0;vr::adaptGunInput(p1,true,x,y,b);
 b=1;vr::adaptGunInput(p1,true,x,y,b);x=y=-10000;b=2;vr::adaptGunInput(p1,true,x,y,b);check(x==3500&&y==4500&&b==1,"held on-screen shot swept into grey ring does not reload");
 x=y=-20000;b=2;vr::adaptGunInput(p1,true,x,y,b);check(x==3500&&y==4500&&b==1,"held on-screen shot swept fully off-screen does not reload");
 b=0;vr::adaptGunInput(p1,true,x,y,b);x=y=-10000;b=2;vr::adaptGunInput(p1,true,x,y,b);x=3500;y=4500;b=1;vr::adaptGunInput(p1,true,x,y,b);check(x<0&&y<0&&b==2,"held off-screen reload swept onto screen never becomes a shot");
 x=3000;y=4500;b=128;vr::adaptGunInput(p1,false,x,y,b);check(x==3000&&y==4500&&b==128,"other games preserve cabinet B");
 x=y=-10000;b=0;vr::adaptGunInput(p1,false,x,y,b);check(x<0&&y<0&&b==0,"other games preserve native off-screen aim");
 vr::GunAxisCalibration axes[4]={{13,228,122},{12,238,124},{20,240,130},{8,232,118}};
 check(vr::gunCalibrationForward(128,640,axes[0])==112 && vr::gunCalibrationForward(320,640,axes[0])==338 && vr::gunCalibrationForward(512,640,axes[0])==570,"P1 measured asymmetric X calibration");
 check(vr::gunCalibrationForward(96,480,axes[1])==84 && vr::gunCalibrationForward(240,480,axes[1])==248 && vr::gunCalibrationForward(384,480,axes[1])==410,"P1 measured asymmetric Y calibration");
 check(vr::gunCalibrationForward(320,640,axes[2])==315 && vr::gunCalibrationForward(240,480,axes[3])==261,"P2 independent measured calibration");
 for(int a=0;a<4;a++){ int map[640]; int extent=(a%2)?480:640; bool good=vr::gunCalibrationInverse(extent,axes[a],map); int worst=0; for(int t=0;t<extent&&good;t++){ if(map[t]<0||map[t]>=extent){good=false;break;} int error=std::abs(vr::gunCalibrationForward(map[t],extent,axes[a])-t);if(error>worst)worst=error; } check(good&&worst<=2,"every game pixel maps within two pixels without changing calibration"); }
 check(vr::gunCalibrationForward(0,640,axes[0])<0 && vr::gunCalibrationForward(0,480,axes[1])<0,"original off-screen ADC sentinel remains outside");
 check(vr::gunCalibrationForward(307,640,axes[0])==318 && vr::gunCalibrationForward(233,480,axes[1])==238,"signed JVS low-byte assembly matches the sampled native inputs");
 int map[640]; check(!vr::gunCalibrationInverse(640,{128,128,128},map),"degenerate live calibration is rejected");
 vr::GunAxisCalibration inverted{240,10,120}; bool mirrored=vr::gunCalibrationInverse(640,inverted,map); for(int t=0;t<640&&mirrored;t++)mirrored=std::abs(vr::gunCalibrationForward(map[t],640,inverted)-t)<=2; check(mirrored,"inverted live axes preserve neutral aim");
 return failed?1:0;
}
