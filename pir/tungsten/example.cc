#include <iostream>

#include "example.h"

void SequenceGenerator::Eval() {
  bool vector = false;
  Token t;
  if (!vector) {
    t.type = TT_INT;
    t.int_ = count;
  } else {
    t.type = TT_INTVEC;
    for (int i = 0; i < 16; i++) {
      t.intVec_[i] = count + i;
    }
  }
  if (out->Ready() && count < 4000) {
    out->Push(t);
    count+=4;
    _pushed = t;
  } else {
    _pushed = std::nullopt;
  }
}

void SequenceGenerator::Log(std::ostream &log) {
  if (_pushed) {
    log << name << " pushed " << _pushed->ToString() << std::endl;
  }
}

void SequenceConsumer::Eval() {
  if (in->Valid()) {
    value = in->Read();
    valid = true;
    in->Pop();
    Complete(1);
  } else {
    valid = false;
  }
}

void SequenceConsumer::Log(std::ostream &log) {
  if (valid)
    log << name << " Read value: " << value.ToString() << std::endl;
  //else
    //log << name << " No value" << std::endl;
}
