
#include "context.h"

Context::Context(const string& _name, const string& _global, const string & _srcCtx) :
  Module(_name),global(_global),srcCtx(_srcCtx) {
    SetInstrumentation();
  }

void Context::AddCtrler(Controller* ctrler) {
  if (!controllers.empty()) {
    controllers.back()->SetChild(ctrler);
  }
  controllers.push_back(ctrler);
}

void Context::EvalControllers() {
  // Eval from inner most controller to outer most
  bool invalid = true;
  bool outready = true;
  for (int i = controllers.size()-1; i >= 0; i--) {
    auto* ctrler = controllers[i];
    ctrler->EvalChildDone();
    ctrler->EvalDone();
    ctrler->EvalPipe();
    invalid &= ctrler->InputsValid();
    outready &= ctrler->OutputsReady();
  }
  if (!invalid) starve += 1;
  if (!outready) stall += 1;
}

void Context::Log(ostream& log) {
  for (auto* controller: controllers) {
    log << controller->sname << " ";
    controller->Log(log);
    log << " ";
  }
}

void Context::DumpState(std::ostream& log) {
  Module::DumpState(log);
  log << ",\"complete\":" << (controllers.size() > 0 && controllers[0]->IsFinished());
  bool valid = true;
  for (auto* c:controllers)
    valid &= c->InputsValid();
  bool ready = true;
  for (auto* c:controllers)
    ready &= c->OutputsReady();
  log << ", \"ins\":" << valid;
  log << ", \"outs\":" << ready;
  log << ", \"starve\":" << starve;
  log << ", \"stall\":" << stall;
  log << ", \"global\": \"" << global + "\"";
}
