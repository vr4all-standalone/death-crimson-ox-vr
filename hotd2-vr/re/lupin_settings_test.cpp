#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <utility>
namespace config { struct Setting { std::string value; std::string get() const { return value; } }; Setting VrP1Hand, VrHaptics; }
constexpr int Left=0, Right=1;
enum class Holds { Controller, Gun, Nothing };
bool menuHowToPlay=false, dualWield=false, p1Armed=false, inCutscene=false;
struct Gun { int hand; bool triggerWas=false,triggerWait=false; long long pullAt=0; bool pointedDown=false,offArmed=false; } guns[2] {{Right},{Left}};
using XrDuration=long long;
constexpr int XR_TYPE_HAPTIC_ACTION_INFO=1, XR_TYPE_HAPTIC_VIBRATION=2;
constexpr float XR_FREQUENCY_UNSPECIFIED=0;
struct XrHapticActionInfo { int type,action=0,subactionPath=0; };
struct XrHapticBaseHeader { int type; };
struct XrHapticVibration { int type; XrDuration duration=0; float frequency=0,amplitude=0; };
int session=0,hapticAction=0,handPaths[2]={0,1},calls=0,lastHand=-1;
XrHapticVibration lastPulse{};
int xrApplyHapticFeedback(int,const XrHapticActionInfo* info,const XrHapticBaseHeader* pulse){++calls;lastHand=info->subactionPath;lastPulse=*reinterpret_cast<const XrHapticVibration*>(pulse);return 0;}
#define XR(name) name
#define NOTICE_LOG(...)
#include "settings_under_test.inc"
int checks=0;
void check(bool okay,const char* message){++checks;if(!okay){std::fprintf(stderr,"FAIL %s\n",message);std::exit(1);}}
int main(){
 for(const auto& entry : {std::pair<const char*,int>{"right",1},{"RIGHT",1},{"left",0},{"LeFt",0},{"",1},{"invalid",1},{" left",1}}){config::VrP1Hand.value=entry.first;check(p1Hand()==entry.second,"P1 hand parsing/default");}
 for(const auto& entry : {std::pair<const char*,int>{"off",0},{"OFF",0},{"0",0},{"light",1},{"LIGHT",1},{"low",1},{"1",1},{"medium",2},{"MeDiUm",2},{"normal",2},{"2",2},{"strong",3},{"STRONG",3},{"3",3},{"",2},{"invalid",2},{"-1",2},{"4",2}}){config::VrHaptics.value=entry.first;check(hapticsLevel()==entry.second,"legacy/word haptic migration");}
 const char* words[]={"off","light","medium","strong"};const float multiplier[]={0,1,1.6f,2.4f};
 for(int level=0;level<4;++level)for(int hand=0;hand<2;++hand){
  config::VrHaptics.value=words[level];calls=0;float duration=haptic(hand,.03f,1.f);
  check(std::abs(duration-.03f*multiplier[level])<.00001f,"shot pulse duration");
  check(calls==(level?1:0),"Off sends no XR pulse");
  if(level){check(lastHand==hand && lastPulse.amplitude==1.f,"shot pulse assigned hand/amplitude");check(std::abs(lastPulse.duration/1e9-.03*multiplier[level])<.00001,"XR pulse duration matches return");
   haptic(hand,.012f,.35f);check(std::abs(lastPulse.amplitude-.35f)<.00001f && std::abs(lastPulse.duration/1e9-.012*multiplier[level])<.00001,"empty click keeps amplitude and scales length");}
 }
 check(holds(Left)==Holds::Controller && holds(Right)==Holds::Controller,"before play has controllers");
 p1Armed=true;check(holds(Right)==Holds::Gun && holds(Left)==Holds::Controller,"single gun follows its hand");
 inCutscene=true;check(holds(Right)==Holds::Nothing,"story gun hand empty");
 dualWield=true;check(holds(Left)==Holds::Nothing && holds(Right)==Holds::Nothing,"both story hands empty");
 menuHowToPlay=true;check(holds(Left)==Holds::Controller && holds(Right)==Holds::Controller,"How to play overrides story emptiness");
 menuHowToPlay=false;inCutscene=false;check(holds(Left)==Holds::Gun && holds(Right)==Holds::Gun,"both guns return after story");
 guns[0].hand=Left;guns[1].hand=Right;check(holds(Left)==Holds::Gun && holds(Right)==Holds::Gun,"swapped dual guns retained");
 dualWield=false;check(holds(Left)==Holds::Gun && holds(Right)==Holds::Controller,"P1 left remains after P2 leaves");
 for(int downMask=0;downMask<4;++downMask){
  dualWield=true;guns[0].hand=Left;guns[1].hand=Right;
  guns[0].pointedDown=(downMask&1)!=0;guns[1].pointedDown=(downMask&2)!=0;
  guns[0].triggerWas=guns[0].triggerWait=guns[1].triggerWas=guns[1].triggerWait=false;
  guns[0].pullAt=guns[1].pullAt=123;guns[0].offArmed=guns[1].offArmed=true;
  syncHands(Right);
  check(guns[0].hand==Right && guns[1].hand==Left,"live swap assigns players");
  check(guns[0].pointedDown==((downMask&2)!=0) && guns[1].pointedDown==((downMask&1)!=0),"downward reload latch stays with physical hand across swap");
  check(guns[0].triggerWas && guns[0].triggerWait && guns[1].triggerWas && guns[1].triggerWait,"swap waits for release");
  check(!guns[0].pullAt && !guns[1].pullAt && !guns[0].offArmed && !guns[1].offArmed,"swap clears pending pulls and point-off-screen arming");
 }
 std::printf("PASS %d production settings, pulse and held-state checks\n",checks);
}