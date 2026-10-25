#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
#include "death_gun_choice_type_under_test.inc"
namespace config {int VrGun=1;bool VrGameHands=true;}
constexpr int OwnGuns=4;
enum class GunKind {Arcade,Pistol,Rifle};
struct HandsModel {} model;
struct Source {} source;
struct OwnGun {std::string name;bool valid;struct {std::string stem;} file;};
std::vector<std::unique_ptr<OwnGun>> ownGuns;
bool selectable=true,sourceKnown=false,gameModelPresent=false,nativeComplete=false;
bool selectableGuns(){return selectable;}
const Source* gameModelSource(){return sourceKnown?&source:nullptr;}
const HandsModel* handsModel(){return gameModelPresent?&model:nullptr;}
const HandsModel* gunModel(GunKind){return gameModelPresent?&model:nullptr;}
bool gunComplete(GunKind){return nativeComplete;}
void refreshOwn(){}
#include "death_gun_choices_under_test.inc"
int checks=0,failed=0;
void check(bool pass,const char* name){++checks;if(!pass){++failed;std::printf("FAIL %s\n",name);}}
void reset(){ownGuns.clear();selectable=true;sourceKnown=gameModelPresent=nativeComplete=false;config::VrGameHands=true;config::VrGun=1;}
void folder(const char* label,const char* stem,bool valid=true){ownGuns.push_back(std::make_unique<OwnGun>(OwnGun{label,valid,{stem}}));}
int main(){
 // Removing model assets must never leave the menu with no valid row to index.
 for(bool gameHands:{false,true}){
  reset();config::VrGameHands=gameHands;const auto choices=gunChoices();
  check(!choices.empty(),"missing native source and model files leaves a selectable built-in fallback");
  if(!choices.empty()){
   check(choices.size()==1&&choices[0].setting==1&&std::strcmp(choices[0].label,"Arcade gun")==0,"empty folder offers exactly the built-in Arcade gun");
   check(choices[0].made&&choices[0].complete&&choices[0].file==nullptr,"built-in fallback needs no external model file");
  }
 }
 reset();folder("Arcade gun","arcade-gun");folder("Zapper","zapper");folder("My pistol","my-pistol");
 auto choices=gunChoices();check(choices.size()==3,"licensed and custom models do not gain a duplicate built-in Arcade gun");
 const char* labels[]={"Arcade gun","Zapper","My pistol"};const char* stems[]={"arcade-gun","zapper","my-pistol"};
 for(size_t i=0;i<choices.size()&&i<3;++i)check(choices[i].setting==4&&choices[i].made&&choices[i].complete&&std::strcmp(choices[i].label,labels[i])==0&&std::strcmp(choices[i].file,stems[i])==0,"folder choices retain order, names and persisted model stems");
 reset();folder("Unavailable model","bad-model",false);choices=gunChoices();
 check(choices.size()==1&&!choices[0].made&&choices[0].file&&std::strcmp(choices[0].file,"bad-model")==0,"existing invalid model metadata retains its unavailable choice without changing fallback policy");
 for(bool gameHands:{false,true})for(bool present:{false,true}){
  reset();sourceKnown=true;config::VrGameHands=gameHands;gameModelPresent=nativeComplete=present;
  choices=gunChoices();check(choices.size()==1&&choices[0].setting==1&&std::strcmp(choices[0].label,"In-game pistol")==0,"verified native source retains its legacy pistol row");
  check(choices[0].made==(gameHands&&present)&&choices[0].complete==present,"native model availability and completion remain unchanged");
 }
 reset();selectable=false;config::VrGun=9;choices=gunChoices();
 check(choices.size()==1&&choices[0].setting==9&&std::strcmp(choices[0].label,"Arcade gun")==0,"nonselectable legacy game retains its locked original setting");
 gameModelPresent=true;choices=gunChoices();
 check(choices.size()==1&&choices[0].setting==9&&std::strcmp(choices[0].label,"The game's own")==0,"nonselectable legacy native model remains unchanged");
 std::printf("%s %d real gunChoices checks (%d failures)\n",failed?"FAIL":"PASS",checks,failed);return failed?1:0;
}