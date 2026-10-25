// Compile real hand policy, updateGun input/release blocks, Option and INI parser code.
// XR transport, poses and drawing are the only substituted boundaries.
#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <locale>
#include <map>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>
#include <glm/glm.hpp>
#define NOTICE_LOG(...) ((void)0)
#define WARN_LOG(...) ((void)0)
#define stricmp _stricmp
namespace hostfs { class File; }
std::string trim_ws(const std::string& text, const char* chars) {
    const auto first=text.find_first_not_of(chars);
    return first==std::string::npos ? std::string() : text.substr(first,text.find_last_not_of(chars)-first+1);
}
#include "death_ini_under_test.inc"
namespace config {
IniFile cfgdb;
bool loadBool(const std::string& s,const std::string& k,bool d=false){return cfgdb.getBool(s,k,d);}
void saveBool(const std::string& s,const std::string& k,bool v){cfgdb.set(s,k,v);}
bool hasSection(const std::string& s){return cfgdb.hasSection(s);}
bool isTransient(const std::string& s,const std::string& k){return cfgdb.isTransient(s,k);}
void deleteSection(const std::string& s){cfgdb.deleteSection(s);}
void deleteEntry(const std::string& s,const std::string& k){cfgdb.deleteEntry(s,k);}
void setAutoSave(bool){}
int loadInt(const std::string&,const std::string&,int);
int64_t loadInt64(const std::string&,const std::string&,int64_t);
float loadFloat(const std::string&,const std::string&,float);
std::string loadStr(const std::string& s,const std::string& k,const std::string& d={}){return cfgdb.getString(s,k,d);}
void saveInt(const std::string&,const std::string&,int);
void saveInt64(const std::string&,const std::string&,int64_t);
void saveFloat(const std::string&,const std::string&,float);
void saveStr(const std::string& s,const std::string& k,const std::string& v){cfgdb.set(s,k,v);}
}
#include "death_option_under_test.inc"
namespace config {
Option<bool> VrXrGun("test.xr",true), VrLaser("test.laser",true), VrDualWield("test.dual",true), GGPOEnable("test.ggpo",false);
}
using XrTime=long long;
using u32=uint32_t;
constexpr int Left=0,Right=1,OffScreen=-10000;
enum class Holds {Controller,Gun,Nothing};
struct GunView {};
namespace vr {struct GunState {int ammo=0;};bool cutscene(){return false;}bool gameAnalogOffscreenReload(){return false;}}
#include "death_gun_under_test.inc"
#include "death_physical_trigger_under_test.inc"
bool dualWield=false,p2StartWas=false,recenterWas=false,laserWas=false,menuWas=false,p1StartWas=false;
bool p1Armed=false,menuHowToPlay=false,inCutscene=false,cutsceneVeto=false;
bool pointerShown[2]{};
bool focused=true,menuOpen=false,skipWas=false,skipSwallow=false,startSwallow=false,ringDumpRequested=false;
XrTime dualSince=0,leftHoldSince=0,leftTapUntil=0;
bool leftHoldMenu=false;
std::atomic<bool> newGame{false};
struct {bool held=false;} slide;
namespace hands {bool preparing(){return false;}void skipPrep(){}}
namespace menu {void open(const glm::mat4&) {}}
void setHowToPlay(bool on,const char*){menuHowToPlay=on;}
void noteHolds(const char*){}
int clearP2=0;
void lightgunSet(int player,int,int,u32 buttons){if(player==1&&!buttons)++clearP2;}
void haptic(int,float,float){}
struct XrActiveActionSet {int action,path;};
struct XrActionsSyncInfo {int type;int countActiveActionSets=0;const XrActiveActionSet* activeActionSets=nullptr;};
constexpr int XR_NULL_PATH=0,XR_TYPE_ACTIONS_SYNC_INFO=0;
int actionSet=0,session=0;
#define XR(name) name
#define XR_SUCCEEDED(value) ((value)==0)
int xrSyncActions(int,const XrActionsSyncInfo*){return 0;}
enum {startAction,menuAction,skipAction,recenterAction,laserAction,triggerAction,ActionCount};
bool buttons[ActionCount][2]{};
float triggerPull[2]{};
bool tracking[2]{true,true};
bool boolAction(int action,int hand=-1){return hand<0?(buttons[action][0]||buttons[action][1]):buttons[action][hand];}
float floatAction(int,int hand){return triggerPull[hand];}
bool menuByHold(){return false;}
void updateMenu(XrTime,bool,bool,bool){}
glm::mat4 headPose(){return glm::mat4(1);}
void recenter(){}
glm::vec2 stickAction2(){return glm::vec2(0);}
bool adjustWorldSize(XrTime,const glm::vec2&){return false;}
u32 dpadFromStick(const glm::vec2&){return 0;}
struct XrSpaceLocation {};
void locateHands(XrTime,XrSpaceLocation*,bool* tracked){tracked[0]=tracking[0];tracked[1]=tracking[1];}
bool promptKnown=true,prompting=false;
bool player2PromptKnown(){return promptKnown;}
bool player2Prompting(){return prompting;}
u32 sentButtons[2]{};
bool productionGunInput=false, testAimOnScreen=true, testAimOutside=false;
const char* aimHow="3D";
void updateGun(Gun&,XrTime,const XrSpaceLocation*,const bool*,const float*,u32,bool);
#include "death_host_under_test.inc"
#include "death_menu_under_test.inc"
namespace controllers { enum Button { A, B, X, Y, Menu, Stick, Trigger, Grip }; }
#include "death_labels_under_test.inc"
#include "death_update_gun_input_under_test.inc"
// One-frame policy checks inspect boundary writes; multi-frame checks execute actual production input blocks.
void updateGun(Gun& gun,XrTime time,const XrSpaceLocation*,const bool*,const float* pulls,u32 b,bool){
 if(productionGunInput)updateGunBoundary(gun,time,pulls,b);else sentButtons[gun.player]=b;
}
int checks=0,failed=0;
void check(bool pass,const char* name){++checks;if(!pass){++failed;std::printf("FAIL %s\n",name);}}
void swapOption(bool on){
#if DEATH_SWAP_OPTION
 config::VrSwapP1P2.set(on);
#else
 config::VrP1Hand.set(on?"left":"right");
#endif
}
void reset(){
 productionGunInput=false;testAimOnScreen=true;testAimOutside=false;
#if PHYSICAL_RELEASE_LATCH
 triggerReleaseWait[0]=triggerReleaseWait[1]=false;
#endif
 guns[0]=Gun{0};guns[1]=Gun{1,Left};
 dualWield=p2StartWas=recenterWas=laserWas=menuWas=p1StartWas=p1Armed=menuHowToPlay=inCutscene=cutsceneVeto=false;
 menuOpen=skipWas=skipSwallow=startSwallow=ringDumpRequested=leftHoldMenu=false;
 dualSince=leftHoldSince=leftTapUntil=0;newGame=false;
 config::VrXrGun=true;config::VrDualWield=true;config::GGPOEnable=false;
 focused=promptKnown=true;prompting=false;clearP2=0;
 std::memset(buttons,0,sizeof(buttons));std::memset(sentButtons,0,sizeof(sentButtons));
 tracking[0]=tracking[1]=true;triggerPull[0]=triggerPull[1]=0;
}
void defaultAndPersistence(){
 check(p1Hand()==Left,"new configuration assigns P1 left by default");
#if DEATH_SWAP_OPTION
 check(config::VrSwapP1P2.get(),"dedicated swap option is enabled by default");
 for(const auto& fixture:std::vector<std::pair<std::string,bool>>{{"yes",true},{"TRUE",true},{"On",true},{"1",true},{"no",false},{"false",false},{"off",false},{"0",false}}){
  config::cfgdb=config::IniFile();
  config::cfgdb.load("[config]\nvr.SwapP1P2 = "+fixture.first+"\n");
  config::VrSwapP1P2.reset();config::VrSwapP1P2.load();
  check(p1Hand()==(fixture.second?Left:Right),"persisted boolean controls physical assignment");
 }
 for(bool enabled:{false,true}){
  config::cfgdb=config::IniFile();config::VrSwapP1P2=enabled;config::VrSwapP1P2.save();
  std::string data;config::cfgdb.save(data);config::cfgdb=config::IniFile();config::cfgdb.load(data);
  config::VrSwapP1P2=!enabled;config::VrSwapP1P2.load();
  check(p1Hand()==(enabled?Left:Right),"saved assignment survives INI round trip");
 }
 config::cfgdb=config::IniFile();config::cfgdb.load("[config]\nvr.P1Hand = right\n");
 config::VrSwapP1P2.reset();config::VrSwapP1P2.load();
 check(p1Hand()==Left,"old single-hand preference cannot override new two-player default");
#else
 check(false,"dedicated two-player boolean persistence exists");
#endif
}
void singleHandPolicy(){
 for(bool enabled:{false,true}){
  reset();swapOption(enabled);p1Armed=true;
  triggerPull[Left]=.8f;updateLightgun(1);
  check(!dualWield&&guns[0].hand==Left,"left trigger moves single gun regardless of option");
  triggerPull[Left]=0;triggerPull[Right]=.8f;updateLightgun(2);
  check(guns[0].hand==Right,"right trigger moves single gun regardless of option");
  guns[0].triggerWas=true;triggerPull[Left]=.9f;triggerPull[Right]=0;updateLightgun(3);
  check(guns[0].hand==Right,"held gun trigger cannot transfer a pull between hands");
  guns[0].triggerWas=false;triggerPull[Left]=triggerPull[Right]=0;tracking[Right]=false;updateLightgun(4);
  check(guns[0].hand==Left,"tracking loss transfers single gun to available controller");
 }
}
void startsAndJoin(){
 for(bool enabled:{false,true})for(int hand:{Left,Right}){
  reset();swapOption(enabled);config::VrDualWield=false;buttons[startAction][hand]=true;updateLightgun(1);
  check(!dualWield&&(sentButtons[0]&4),"either Start starts P1 when two-player input is disabled");
  reset();swapOption(enabled);tracking[1-hand]=false;buttons[startAction][hand]=true;updateLightgun(1);
  check(!dualWield&&(sentButtons[0]&4),"single tracked controller can start P1 in either assignment");
 }
 for(bool enabled:{false,true}){
  reset();swapOption(enabled);const int hand1=enabled?Left:Right,hand2=1-hand1;
  buttons[startAction][hand1]=true;updateLightgun(1);
  check(!dualWield&&(sentButtons[0]&4),"assigned P1 Start starts without joining P2");
  buttons[startAction][hand1]=false;
  guns[0].hand=hand2;guns[1].hand=hand1;guns[0].pointedDown=true;
  guns[0].shotCount=31;guns[0].gunWas.ammo=7;guns[0].pullAt=123;guns[0].shotsDue=2;
  triggerPull[0]=triggerPull[1]=.9f;buttons[startAction][hand2]=true;updateLightgun(2);
  check(dualWield&&guns[0].hand==hand1&&guns[1].hand==hand2,"P2 Start joins with chosen two-player assignment");
  check((sentButtons[1]&4)&&!(sentButtons[0]&4),"join Start reaches the actual P2 game input");
  check(guns[0].triggerWait&&guns[1].triggerWait,"joining suppresses held triggers for both players");
  check(!guns[0].pullAt&&!guns[0].shotsDue,"joining cannot carry a pending pull into a new controller");
  check(guns[0].shotCount==31&&guns[0].gunWas.ammo==7,"joining keeps P1 shot and native player state");
  check(!guns[0].pointedDown&&guns[1].pointedDown,"joining retains downward latch on its physical controller");
 }
}
void liveSwapAndLeave(){
 for(bool enabled:{false,true})for(int mask=0;mask<4;++mask){
  reset();swapOption(!enabled);dualWield=p1Armed=true;dualSince=1;
  guns[0].hand=enabled?Right:Left;guns[1].hand=1-guns[0].hand;
  guns[0].pointedDown=(mask&1)!=0;guns[1].pointedDown=(mask&2)!=0;
  guns[0].shotCount=27;guns[1].shotCount=14;guns[0].gunWas.ammo=5;guns[1].gunWas.ammo=3;
  guns[0].pullAt=guns[1].pullAt=55;guns[0].offArmed=guns[1].offArmed=true;
  swapOption(enabled);triggerPull[0]=triggerPull[1]=.9f;updateLightgun(2);
  check(guns[0].hand==(enabled?Left:Right)&&guns[1].hand==(enabled?Right:Left),"live toggle reverses physical assignment");
  check(guns[0].pointedDown==((mask&2)!=0)&&guns[1].pointedDown==((mask&1)!=0),"live swap keeps downward latch with physical hand");
  check(guns[0].triggerWas&&guns[0].triggerWait&&guns[1].triggerWas&&guns[1].triggerWait,"live swap waits for release on both guns");
  check(!guns[0].pullAt&&!guns[1].pullAt&&!guns[0].offArmed&&!guns[1].offArmed,"live swap clears stale pull and off-screen reload arming");
  check(guns[0].shotCount==27&&guns[1].shotCount==14&&guns[0].gunWas.ammo==5&&guns[1].gunWas.ammo==3,"live toggle retains each player's gameplay state");
  config::VrDualWield=false;guns[0].triggerWait=false;guns[0].pullAt=88;updateLightgun(3);
  check(!dualWield&&clearP2==1,"leaving releases P2 game input");
  check(guns[0].triggerWait&&!guns[0].pullAt,"leaving suppresses held P1 trigger before single play");
 }
 reset();swapOption(true);dualWield=p1Armed=true;guns[0].hand=Left;guns[1].hand=Right;
 newGame=true;triggerPull[0]=triggerPull[1]=.9f;updateLightgun(1);
 check(!dualWield&&!p1Armed&&holds(Left)==Holds::Controller&&holds(Right)==Holds::Controller,"new game restores controllers and labels");
 check(guns[0].triggerWait,"new game suppresses a held trigger until released");
 check(guns[1].hand!=guns[0].hand,"new game keeps controller reload latches on distinct hands");
}
void menuAndLabels(){
 for(bool enabled:{false,true}){
  reset();swapOption(enabled);toggleHands();
  check(p1Hand()==(enabled?Right:Left),"actual menu toggle changes assignment");
#if DEATH_SWAP_OPTION
  config::VrSwapP1P2=enabled;config::VrSwapP1P2.load();
  check(p1Hand()==(enabled?Right:Left),"actual menu toggle persists assignment immediately");
#endif
  const auto& words=wordsFor("/interaction_profiles/oculus/touch_controller");
  for(int hand:{Left,Right})for(const auto& label:words.hand[hand]){
   if(label.button==controllers::X||label.button==controllers::A||label.button==controllers::Y||label.button==controllers::B)
    check(std::strstr(label.text,hand==p1Hand()?"P1":"P2")!=nullptr,"controller Start/reload labels follow physical player assignment");
   if(label.button==controllers::Trigger)
    check(std::strstr(label.text,"solo")!=nullptr,"controller trigger label explains single-player either-hand behavior");
  }
 }
}
void perPlayerButtonsAndLifecycle(){
 for(bool enabled:{false,true})for(int hand:{Left,Right}){
  reset();swapOption(enabled);p1Armed=true;buttons[skipAction][hand]=true;updateLightgun(1);
  check((sentButtons[0]&128)&&!sentButtons[1],"either solo reload button reaches P1 independently of assignment");
  reset();swapOption(enabled);dualWield=p1Armed=true;dualSince=1;
  guns[0].hand=enabled?Left:Right;guns[1].hand=1-guns[0].hand;
  buttons[skipAction][hand]=true;updateLightgun(2);
  const bool leftIsP1=enabled;
  const bool reloadP1=(hand==Left)==leftIsP1;
  check(((sentButtons[0]&128)!=0)==reloadP1&&((sentButtons[1]&128)!=0)!=reloadP1,"dual reload buttons reach the player assigned to that controller");
  for(int unavailable=0;unavailable<2;++unavailable){
   reset();swapOption(enabled);buttons[startAction][hand]=true;
   if(unavailable==0)promptKnown=false;else config::GGPOEnable=true;
   updateLightgun(1);
   check(!dualWield&&(sentButtons[0]&4),"unsupported or online two-player mode leaves either P1 Start usable");
  }
 }
 reset();swapOption(true);dualWield=p1Armed=true;dualSince=1;guns[0].hand=Left;guns[1].hand=Right;prompting=true;
 updateLightgun(2'000'000'000LL);
 check(dualWield,"P2 prompt cannot cancel join before the native game responds");
 updateLightgun(4'000'000'002LL);
 check(!dualWield&&clearP2==1,"P2 native leave prompt returns to one gun after grace period");
 // A single gun may move repeatedly before the second player joins. The inactive gun
 // remembers the other physical controller's latch rather than duplicating P1's hand.
 reset();swapOption(true);p1Armed=true;guns[0].pointedDown=true;
 triggerPull[Left]=.9f;updateLightgun(1);
 check(guns[0].hand==Left&&guns[1].hand==Right&&!guns[0].pointedDown&&guns[1].pointedDown,"solo switch leaves downward reload latch on the physical controller");
 triggerPull[Left]=0;buttons[startAction][Right]=true;updateLightgun(2);
 check(dualWield&&!guns[0].pointedDown&&guns[1].pointedDown,"join after a solo switch preserves both controller reload latches");
}
void heldPhysicalTriggerAcrossFrames(){
 for(bool enabled:{false,true})for(bool onScreen:{false,true})for(int boundary=0;boundary<5;++boundary){
  reset();swapOption(enabled);productionGunInput=true;testAimOnScreen=onScreen;p1Armed=true;
  const int p1=enabled?Left:Right,p2=1-p1;
  guns[0].hand=p1;guns[1].hand=p2;
  if(boundary==0){dualWield=true;config::VrDualWield=false;}
  if(boundary==1){buttons[startAction][p2]=true;}
  if(boundary==2){dualWield=true;swapOption(!enabled);}
  if(boundary==3){dualWield=true;newGame=true;}
  if(boundary==4){dualWield=true;prompting=true;}
  const XrTime base=5'000'000'000LL;
  triggerPull[p2]=.9f;triggerPull[p1]=0;
  updateLightgun(base+10);
  // P1's released controller clears its per-player latch. That must not authorize
  // the still-held physical P2 trigger to become a fresh P1 pull after leaving.
  config::VrDualWield=false;buttons[startAction][p2]=false;
  for(int frame=11;frame<=14;++frame){
   updateLightgun(base+frame);
   check(!(sentButtons[0]&3),"held physical trigger never fires/reloads through join/swap/leave frames");
  }
  tracking[p2]=false;triggerPull[p2]=0;updateLightgun(base+15);
  tracking[p2]=true;triggerPull[p2]=.9f;updateLightgun(base+16);
  check(!(sentButtons[0]&3),"untracked controller cannot count as the physical trigger release");
  triggerPull[p2]=.4f;updateLightgun(base+17);updateLightgun(base+18);
  check(!(sentButtons[0]&3),"half-released inactive trigger remains suppressed after transfer boundary");
  triggerPull[p2]=.35f;updateLightgun(base+19);
  check(!(sentButtons[0]&3),"releasing inactive trigger clears suppression without a shot");
  triggerPull[p2]=.9f;updateLightgun(base+20);
  check(guns[0].hand==p2&&(sentButtons[0]&3)==(onScreen?1u:2u),"fresh press after physical release transfers and shoots/reloads normally");
 }
}
int main(){defaultAndPersistence();singleHandPolicy();startsAndJoin();liveSwapAndLeave();menuAndLabels();perPlayerButtonsAndLifecycle();heldPhysicalTriggerAcrossFrames();
 std::printf("%s %d production hand-policy and INI checks (%d failures)\n",failed?"FAIL":"PASS",checks,failed);return failed?1:0;}