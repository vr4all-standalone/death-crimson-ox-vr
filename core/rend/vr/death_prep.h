// First-run scheduling is separate from the game decoder and filesystem boundaries.
#pragma once
#include <exception>
#include <atomic>
#include <string>
#include <vector>
#include <cstdint>
namespace vr::hands {
class DeathPreparation {
public:
 enum class Phase { Off, Queued, Working, Ready, Skipped, Failed };
 Phase phase = Phase::Off;
 std::string error;
 bool needsPresentation=false;
 std::atomic<bool> framePresented{false};
 void begin(bool enabled, bool cached, bool presentationRequired = false) {
  needsPresentation=presentationRequired;framePresented=false;
  error.clear(); phase=!enabled?Phase::Off:cached?Phase::Ready:Phase::Queued;
 }
 void presented() {framePresented=true;}
 bool preparing() const { return phase==Phase::Queued || phase==Phase::Working; }
 template<class Build,class Save> bool tick(bool skip,Build build,Save save) {
  if(!preparing()) return false;
  if(skip){phase=Phase::Skipped;return false;}
  if(needsPresentation&&!framePresented.load()) return false;
  if(phase==Phase::Queued){phase=Phase::Working;return false;}
  std::vector<uint8_t> data;
  try {
   if(!build(data,error)||!save(data,error)){phase=Phase::Failed;return false;}
  } catch(const std::exception& e) {error=e.what();phase=Phase::Failed;return false;}
  phase=Phase::Ready; return true;
 }
};
}
