#include <cstdio>
#include <cstdint>
using u16=std::uint16_t; using s16=std::int16_t; using s8=std::int8_t;
struct Snapshot { bool valid=true; int ammo=10; u16 shots=0,fired=0; bool rifle=false; };
Snapshot step(Snapshot was,Snapshot now,bool isDeath) {
#include "feedback_under_test.inc"
return now;
}
int main(){int checks=0,failed=0;
auto test=[&](bool ok,const char* why){++checks;if(!ok){++failed;std::printf("FAIL %s\n",why);}};
Snapshot a,b;
b=a;b.ammo=9;b=step(a,b,true);test(b.fired==0,"ammo tick alone does not invent firing feedback");
a=b;b.shots=1;b=step(a,b,true);test(b.fired==1,"later accepted-shot tick fires exactly once");
a=b;b=step(a,b,true);test(b.fired==1,"stable counter does not repeat feedback");
a=b;b.ammo=10;b=step(a,b,true);test(b.fired==1,"reload produces no firing feedback");
a=b;b.ammo=9;b.shots=2;b=step(a,b,true);test(b.fired==2,"same-frame accepted round also works");
a=b;a.ammo=0;a.shots=10;a.fired=10;b=a;b=step(a,b,true);test(b.fired==10,"empty pull without accepted round has no recoil");
a=b;b.valid=false;b.shots=0;b=step(a,b,true);test(b.fired==10,"inactive/reset produces no kick");
a=Snapshot{};b=a;b.shots=1;b=step(a,b,false);test(b.fired==0,"legacy game does not infer accepted rounds from raw pulls");
a=Snapshot{};b=a;b.ammo=9;b.shots=1;b=step(a,b,false);test(b.fired==1,"legacy same-frame ammo evidence preserved");
a=Snapshot{};b=a;b.shots=1;b.rifle=true;b=step(a,b,false);test(b.fired==1,"legacy automatic weapon evidence preserved");
a=Snapshot{};b=a;b.shots=99;b=step(a,b,true);test(b.fired==0,"implausible reset/jump rejected");
std::printf("%d feedback timing checks, %d failures\n",checks,failed);return failed?1:0;}
