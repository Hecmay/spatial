#include "controller.h"

Controller::Controller(const string& _name) :
  Module(_name) {
    sname = _name;
    sname = replace(sname, "Controller", "C");
    sname = replace(sname, "Top", "T");
    sname = replace(sname, "Unit", "U");
    sname = replace(sname, "Loop", "L");
  }

void Controller::AddInput(CheckedReceive<Token>* _input, bool OutOfBand) {
  inputs.push_back(_input);
  inputs_oob.push_back(OutOfBand);
}

void Controller::AddOutput(CheckedReady* _output) {
  outputs.push_back(_output);
}

void Controller::AddOutput(CheckedSend<Token>* _output, ValReadyPipeline<Token> *pipe) {
  outpipes.push_back(make_tuple(pipe, _output));
  //outputs.push_back(_output);
}

bool Controller::InputsValid() {
  bool valid = true;
  for (auto* in: inputs)
    valid &= in->Valid();
  return valid;
}

bool Controller::InputsAllLast() {
  bool last = true;
  for (auto* in: inputs)
    last &= (bool) in->Read().done_vec;
  return last;
}

bool Controller::OutputsReady() {
  bool ready = true;
  for (auto* out: outputs)
    ready &= out->Ready();
  for (auto& [pipe, out]: outpipes) {
    ready &= pipe->Ready();
  }
  return ready;
}

bool Controller::PipeExistsValid() {
  bool pipeExistsValid = false;
  for (auto& [pipe, out]: outpipes) {
    pipeExistsValid |= pipe->Valid();
  }
  return pipeExistsValid;
}

void Controller::EvalPipe() {
  bool outAllReady = true;
  for (auto& [pipe, out]: outpipes) {
    outAllReady &= out->Ready();
  }
  if (outAllReady) {
    for (auto& [pipe, out] : outpipes) {
      if (pipe->Valid()) out->Push(pipe->Read());
    }
  } else {
    bool pipeExistsValid = PipeExistsValid();
    if (pipeExistsValid) {
      for (auto& [pipe, out]: outpipes) {
        pipe->Stall();
      }
    }
  }
}

void Controller::SetChild(Controller* _child) {
  child = _child;
  child->parent = this;
}

bool Controller::Done() const {
  return _done;
}

bool Controller::TileDone() const {
  return _tile_done;
}

bool Controller::SubTileDone() const {
  return _sub_tile_done;
}

bool Controller::IsFinished() { return false; };

void Controller::SetEn(bool en) {
  _en = en;
}

bool Controller::Enabled() {
  if (_update_enabled) 
    return _enabled;
  _enabled = EvalEnabled() && !_force_disable_count;
  _update_enabled = true;
  return _enabled;
}

/* bool Controller::ForceDisabled() {
  if (_update_enabled) return _force_disable;
  EvalEnabled();
  _update_enabled = true;
  return _force_disable;
} */

void Controller::ForceDisable() {
  _force_disable_count++;
  if (child)
    child->ForceDisable();
}

void Controller::ForceReenable() {
  _force_disable_count--;
  assert(_force_disable_count >= 0);
  // cout << path << name << " reenable" << endl;
  if (child)
    child->ForceReenable();
}

bool Controller::EvalEnabled() {
  bool en = _en;
  if (parent!= NULL) en &= parent->Enabled();
  en &= InputsValid();
  en &= OutputsReady();
  return en;
}

void Controller::Clock() {
  _enabled = false;
  _update_enabled = false;
  _en = true;
  if (_reset) {
    _done = false;
    _childDone = false;
  }
  _reset = false;
}

bool Controller::Stepped() {
  bool v = Enabled();
  if (child != NULL) {
    v &= child->Done();
  }
  return v;
}

bool Controller::ChildDone() const {
  return _childDone;
}

void Controller::EvalChildDone() {
  bool v = Stepped();
  if (v) {
    v &= InputsAllLast();
  }
  _childDone = v;
}

void Controller::EvalDone () {}

void Controller::ResetDescedent() {
  if (child != NULL) {
    child->Reset();
    child->ResetDescedent();
  }
}

void Controller::Log(ostream& log) {
  log << "e:" << _enabled << " cd:" << _childDone << " d:" << _done;
  log << " r:" << _reset;
  log << " oob:" << _oob_valid;
  log << " fd:" << _force_disable;
  log << " td:" << _tile_done;
  log << " std:" << _sub_tile_done;
  if (!inputs.empty()) log << " i:";
  for (auto* in: inputs)
    log << in->Valid();
  if (!outputs.empty()) log << " o:";
  for (auto* out:outputs)
    log << out->Ready();
  if (!outpipes.empty()) log << " go/p:";
  for (auto& [pipe, out]: outpipes)
    log << out->Ready() << "/" << pipe->Ready() << " ";
}

void Controller::Reset() {
  _reset = true;
}

vector<bool>& Controller::LaneValids() {
  return laneValids;
}

void Controller::EvalLaneValids() {
  laneValids.clear();
  laneValids.push_back(Enabled());
};

UnitController::UnitController(const string& _name) : 
  Controller(_name) {}

void UnitController::EvalDone() {
  _done = !_en || ChildDone();
  if (_done) {
    ResetDescedent();
  }
}

StepController::StepController(const string& _name) : 
  Controller(_name) {}

bool StepController::IsFinished() { return _finished; };

bool StepController::EvalEnabled() {
  return !_finished & Controller::EvalEnabled();
}

void StepController::Log(ostream& log) {
  log << "f: " << _finished << " ";
  Controller::Log(log);
}

void StepController::Clock() {
  if (_done) {
    _finished = true;
  }
  if (_reset) {
    _finished = false;
  }
  Controller::Clock();
}
void StepController::EvalDone() {
  _done = !_en || ChildDone();
  if (_done) {
    ResetDescedent();
  }
}

LoopController::LoopController(const string& _name): Controller(_name) {}

void LoopController::AddCounter(CounterLike* ctr) {
  counters.push_back(ctr);
}

void LoopController::SetStop(CheckedReceive<Token>* fifo) {
  stop_fifo = fifo;
}

bool LoopController::EvalStop() {
  if (!stop_fifo.has_value()) return false;
  if (!stop_fifo.value()->Valid()) return false;
  return toT<bool>(stop_fifo.value()->Read(), 0);
}

void LoopController::EvalDone() {
  _done = false;
  _stop = EvalStop();
  if (_stop && _enabled_after_stop) {
    _done = true;
  } else if (!_en) {
    _done = true;
  } else {
    bool v = ChildDone();
    _done = v;
    int C = counters.size();
    // Evaluate Inc of counters from inner loop to outer loop (right to left)
    for (int i = (C-1); i >= 0; i--) {
      bool inc = v;
      if (i != C-1) {
        for (int j = i+1; j < C; j++)
          inc &= counters[j]->Done();
      }
      if (inc) counters[i]->Inc();
    }
    for (auto ctr : counters) {
      _done &= ctr->Done();
    }
  }
  if (_done) {
    ResetDescedent();
    for (auto ctr : counters) {
      //cout << name << " reset " << ctr->name << endl;
      ctr->Reset();
    }
  }
}

void LoopController::Clock() {
  if (_stop)
    _enabled_after_stop = false;
  if (_enabled)
    _enabled_after_stop = true;
  Controller::Clock();
  _stop = false;
}

void LoopController::Log(ostream& log) {
  Controller::Log(log);
  log << " s:" << _stop;
  log << " es:" << _enabled_after_stop;
  log << " fi:" << FirstIter() << " ";
  for (auto* ctr : counters) {
    log << ctr->sname << " ";
    // ctr->ShortLog(log);
    ctr->Log(log);
  }
  log << " lv:[";
  for (bool valid: laneValids)
    log << valid;
  log << "]";
}

bool LoopController::FirstIter() const {
  bool firstIter = true;
  for (auto* ctr : counters) {
    firstIter &= (ctr->count == 0);
    firstIter &= (ctr->tile_count == 0);
  }
  return firstIter;
}

void LoopController::EvalLaneValids() {
  laneValids.clear();
  std::deque<bool> queue;
  queue.push_back(true);
  auto* last = counters.back();
  for (auto* ctr:counters) {
    int size = queue.size();
    bool isLast = ctr == last;
    for (int i = 0; i < size; i++) {
      bool prev = queue.front();
      queue.pop_front();
      for (bool v: ctr->ValidVec()) {
        if (isLast) laneValids.push_back(prev & v);
        else queue.push_back(prev & v);
      }
    }
  }
}

SplitController::SplitController(const string& _name): Controller(_name), _mask_done(16,false) {
}

void SplitController::EvalDone() {
  _done = !_en || ChildDone();
  _done &= _mask_done.back();
  if (_done) {
    ResetDescedent();
  }
}
void SplitController::SetMask(bool* mask_done, int vec) {
  _mask_done.clear();
  for (int i = 0; i < vec; i++)
    _mask_done.push_back(mask_done[i]);
}
void SplitController::Clock() {
  if (_reset) {
    _mask_done.clear();
    _mask_done.emplace_back(false);
  }
  Controller::Clock();
}

void SplitController::EvalLaneValids() {
  laneValids.clear();
  auto* loop = dynamic_cast<LoopController*>(parent);
  if (loop) {
    loop->EvalLaneValids();
    auto& parentValids = loop->LaneValids();
    laneValids.insert(laneValids.begin(), parentValids.begin(), parentValids.end());
    if(_mask_done.size() != laneValids.size())
      throw module_error(this, "Unmatch lane valid width " + to_string(_mask_done.size()) + " " + to_string(laneValids.size()));
    for (int i = 0; i < _mask_done.size(); i++) {
      laneValids[i] = laneValids[i] & _mask_done[i];
    }
  } else {
    for (int i = 0; i < _mask_done.size(); i++) {
      laneValids.push_back(_mask_done[i]);
    }
  }
}

void SplitController::Log(ostream& log) {
  Controller::Log(log);
  log << " lv:[";
  for (bool valid: laneValids)
    log << valid;
  log << "]";
  log << " md:[";
  for (bool done: _mask_done)
    log << done;
  log << "]";
}

