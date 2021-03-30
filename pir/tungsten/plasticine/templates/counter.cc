#include "counter.h"

CounterLike:: CounterLike(const string& _name): 
  Module(_name) {
    sname = _name;
    sname = replace(sname, "Counter", "ctr");
  }

void CounterLike:: Inc() {
  _inc = true;
  _updated = false;
}

bool CounterLike::isInc() {
  return _inc;
}

void CounterLike::Reset() {
  _reset = true;
}

void CounterLike::TileReset() {
  _tile_reset = true;
}

bool CounterLike::Done() {
  if (!_updated) Eval();
  return _done;
}

bool CounterLike::TileDone(){
  if (!_updated) Eval();
  return _tile_done;
}

bool CounterLike::SubTileDone(){
  if (!_updated) Eval();
  return _sub_tile_done;
}
bool CounterLike::ForceDisable() {
  if (!_updated) Eval();
  return _force_disable;
}

void CounterLike::ShortLog(ostream& log) {};
