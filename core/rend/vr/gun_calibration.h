// Original Lupin cabinet calibration inverse. GPL-2.0-or-later.
#pragma once
#include <cstdlib>
namespace vr {
struct GunAxisCalibration { int minimum, maximum, center; };
inline bool validGunCalibration(const GunAxisCalibration& a) {
 return a.minimum>=0 && a.maximum>=0 && a.center>=0 && a.minimum<=255 && a.maximum<=255 && a.center<=255
  && ((a.minimum<a.center&&a.center<a.maximum)||(a.maximum<a.center&&a.center<a.minimum));
}
inline int gunCalibrationForward(int pixel, int extent, const GunAxisCalibration& axis) {
 // Native JVS reader assembles a signed low byte: 0x7afd becomes 0x79fd.
 // Preserve that measured behavior in the inverse rather than patching the game.
 const int adc=pixel*65535/(extent-1);
 const int raw=((adc-((adc&128)?256:0))&65535)>>8;
 const bool upper=(axis.maximum>axis.minimum)==(raw>=axis.center);
 const int numerator=(extent/2)*(raw-axis.center)*(upper?1:-1);
 const int denominator=(upper?axis.maximum:axis.minimum)-axis.center;
 return extent/2+numerator/denominator;
}
inline bool gunCalibrationInverse(int extent, const GunAxisCalibration& axis, int* out) {
 if (!out || extent<2 || extent>640 || !validGunCalibration(axis)) return false;
 int projected[640];
 for(int p=0;p<extent;p++) projected[p]=gunCalibrationForward(p,extent,axis);
 // Only calibration changes rebuild this table. Cache the forward results so the
 // search is bounded comparisons, not hundreds of thousands of integer divisions.
 for(int target=0;target<extent;target++) {
  int best=0,error=0x7fffffff,displacement=0x7fffffff;
  for(int p=0;p<extent;p++) {
   const int e=std::abs(projected[p]-target),d=std::abs(p-target);
   if(e<error || (e==error&&d<displacement)) { best=p;error=e;displacement=d; }
  }
  out[target]=best;
 }
 return true;
}
}
