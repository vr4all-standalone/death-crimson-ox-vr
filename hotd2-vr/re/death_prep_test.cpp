#include "death_prep.h"
#include <cstdio>
#include <stdexcept>
using vr::hands::DeathPreparation;
int main(){int tests=0,failed=0,builds=0,saves=0;DeathPreparation p;using P=DeathPreparation::Phase;
 auto check=[&](bool b,const char* s){++tests;if(!b){++failed;std::printf("FAIL %s\n",s);}};
 auto build=[&](std::vector<uint8_t>& v,std::string&){++builds;v={1,2,3};return true;};
 auto save=[&](const std::vector<uint8_t>& v,std::string&){++saves;return v==std::vector<uint8_t>({1,2,3});};
 p.begin(false,false);check(p.phase==P::Off&&!p.preparing(),"disabled native models schedule no extraction");
 p.begin(true,true);check(p.phase==P::Ready&&!p.preparing(),"validated cache skips first-run work");
 check(!p.tick(false,build,save)&&builds==0&&saves==0,"cache reuse performs no reader/writer calls");
 p.begin(true,false);check(p.preparing(),"missing or corrupt cache schedules first-run work");
 check(!p.tick(false,build,save)&&p.phase==P::Working&&builds==0,"first tick allows presentation before bounded extraction");
 check(p.tick(false,build,save)&&p.phase==P::Ready&&builds==1&&saves==1,"second tick builds and commits exact payload once");
 check(!p.tick(false,build,save)&&builds==1&&saves==1,"ready ticks never duplicate extraction");
 p.begin(true,false);check(!p.tick(true,build,save)&&p.phase==P::Skipped&&builds==1,"skip before extraction does not read game bytes");
 p.begin(true,false);p.tick(false,build,save);check(!p.tick(true,build,save)&&p.phase==P::Skipped&&saves==1,"skip after presentation commits nothing");
 p.begin(true,false);p.tick(false,build,save);
 check(!p.tick(false,[](auto&,auto& e){e="invalid source";return false;},save)&&p.phase==P::Failed&&p.error=="invalid source"&&saves==1,"decoder failure preserves fallback and its useful error");
 p.begin(true,false);p.tick(false,build,save);
 check(!p.tick(false,build,[](const auto&,auto& e){e="write failed";return false;})&&p.phase==P::Failed&&p.error=="write failed","failed write is not reported as ready");
 p.begin(true,false);check(p.error.empty()&&p.preparing(),"new launch retries failures and clears stale error");
 p.tick(false,build,save);check(p.tick(false,build,save)&&p.phase==P::Ready,"retry can complete normally");
 p.begin(true,false);p.tick(false,build,save);
 check(!p.tick(false,[](auto&,auto&)->bool{throw std::runtime_error("broken decoder");},save)&&p.phase==P::Failed&&p.error=="broken decoder","bounded decoder exception uses fallback");
 p.begin(false,true);check(p.phase==P::Off&&!p.preparing(),"disabled models do not run even with a cache");
 const int beforeBuilds=builds,beforeSaves=saves;
 p.begin(true,false,true);
 check(p.preparing()&&p.preparing(),"querying preparation is not presentation acknowledgement");
 p.tick(false,build,save);p.tick(false,build,save);
 check(p.phase==P::Queued&&builds==beforeBuilds&&saves==beforeSaves,"no extraction before a focused rendered panel with sampled actions");
 p.presented();p.tick(false,build,save);
 check(p.tick(false,build,save)&&p.phase==P::Ready,"submitted preparation panel permits the bounded extraction");
 p.begin(true,false,true);
 check(!p.tick(true,build,save)&&p.phase==P::Skipped,"held skip is honored even before first panel submission");
 std::printf("%d preparation checks, %d failures\n",tests,failed);return failed?1:0;
}
