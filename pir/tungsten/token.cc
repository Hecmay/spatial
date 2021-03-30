#include "token.h"

#include <cassert>
#include <sstream>
#include <iomanip>

std::ostream& operator<<(std::ostream& stream, Token token) {
  stream << token.ToString();
  return stream;
}

std::ostream& operator<<(std::ostream& stream, Packet packet) {
  stream << packet.ToString();
  return stream;
}

std::ostream& operator<<(std::ostream& stream, Credit credit) {
  return stream;
}

std::string Token::ToString() const {
  std::string output = "";
  switch(type) {
    case TT_FLOATVEC:
      output += "[";
      for (int i = 0; i < nvalid; i++) {
        std::stringstream val;
        val << std::fixed << std::setprecision(3) << floatVec_[i];
        output += val.str() + " ";
      }
      output += "]";
      break;
    case TT_FLOAT:
      {
        std::stringstream val;
        val << std::fixed << std::setprecision(3) << float_;
        output += val.str();
      }
      break;
    case TT_INT:
      output += std::to_string(int_);
      break;
    case TT_UINT:
      output += std::to_string(uint_);
      break;
    case TT_UINTVEC:
      output += "[";
      for (int i = 0; i < nvalid; i++) {
        char tmp[200];
        sprintf(tmp, "%10u", uintVec_[i]);
        output += string(tmp) + " ";
      }
      output += "]";
      break;
    case TT_INTVEC:
      if (special == "pack3") {
        output += "[";
        for (int i = 0; i < nvalid; i++) {
          char tmpA[200];
          char tmp[200];
          int a = intVec_[i] & ((1<<10)-1);
          int b = (intVec_[i]>>10) & ((1<<10)-1);
          int c = (intVec_[i]>>20) & ((1<<10)-1);
          sprintf(tmp, "%d:%d:%d", a, b, c);
          sprintf(tmpA, "%12s", tmp);
          output += string(tmpA) + " ";
        }
        output += "]";
      } else {
        output += "[";
        for (int i = 0; i < nvalid; i++) {
          char tmp[200];
          sprintf(tmp, "%10d", intVec_[i]);
          output += string(tmp) + " ";
        }
        output += "]";
      }
      break;
    case TT_INT8VEC:
      output += "[";
      for (int i = 0; i < nvalid; i++) {
        output += std::to_string(int8Vec_[i]) + " ";
      }
      output += "]";
      break;
    case TT_INT16VEC:
      output += "[";
      for (int i = 0; i < nvalid; i++) {
        output += std::to_string(int16Vec_[i]) + " ";
      }
      output += "]";
      break;
    case TT_UINT64:
      output += std::to_string(uint64_);
      break;
    case TT_UINT64VEC:
      output += "[";
      for (int i = 0; i < nvalid; i++) {
        output += std::to_string(uint64Vec_[i]) + " ";
      }
      output += "]";
      break;
    case TT_LONG:
      output += std::to_string(long_);
      break;
    case TT_LONGVEC:
      output += "[";
      for (int i = 0; i < nvalid; i++) {
        output += std::to_string(longVec_[i]) + " ";
      }
      output += "]";
      break;
    case TT_BOOLVEC:
      output += "[";
      for (int i = 0; i < nvalid; i++) {
        output += std::to_string(boolVec_[i]) + " ";
      }
      output += "]";
      break;
    case TT_BOOL:
      output += bool_ ? "true" : "false";
      break;
    default:
      assert(false);
      break;
  }

  switch(type) {
    case TT_FLOATVEC:
    case TT_INTVEC:
    case TT_UINTVEC:
    case TT_UINT64VEC:
    case TT_LONGVEC:
    case TT_BOOLVEC:
      output += " v:" + std::to_string(nvalid) + " l:" + std::to_string(last);
    default: break;
  }

  output += " L:" + std::to_string(done_vec);
  return output;
}

int Token::GetValid() const {
  return accumulate(valid.begin(),
      valid.end(), 
      0,
      [](int x, int y) {return x + y;});
}

Token::Token() :
  sender("INVALID"),
  seq_num(-1) {}

Token:: Token(const char* _sender, int _seq_num) :
  sender(_sender),
  seq_num(_seq_num) {}

Packet::Packet() : flow(-1) {
}
Packet::Packet(const Token& _data, int _flow) :
  data(_data), flow(_flow) {
}
Packet::Packet(const Token& _data, int _flow, int _vc) :
  data(_data), flow(_flow), vc(_vc) {
}
std::string Packet::ToString() const {
  std::string output = "<";
  output += std::to_string(flow);
  output += ":";
  output += std::to_string(vc);
  output += " VAL:";
  output += data.ToString();
  output += ">";
  return output;
}

Credit::Credit(int v) {
  vcs.insert(v);
}
Credit::Credit() {}
void Credit::Add(int v) {
  vcs.insert(v);
}

TokenSlice::operator bool&() {
    return t->boolVec_[i];
}
TokenSlice::operator long&() {
    return t->longVec_[i];
}
TokenSlice::operator uint64_t&() {
    return t->uint64Vec_[i];
}
TokenSlice::operator int8_t&() {
    return t->int8Vec_[i];
}
TokenSlice::operator int16_t&() {
    return t->int16Vec_[i];
}
TokenSlice::operator int32_t&() {
    return t->intVec_[i];
}
TokenSlice::operator uint32_t&() {
    return t->uintVec_[i];
}
TokenSlice::operator float&() {
    return t->floatVec_[i];
}

ConstTokenSlice::operator const bool&() const {
    return t->boolVec_[i];
}
ConstTokenSlice::operator const long&() const {
    return t->longVec_[i];
}
ConstTokenSlice::operator const uint64_t&() const {
    return t->uint64Vec_[i];
}
ConstTokenSlice::operator const int8_t&() const {
    return t->int8Vec_[i];
}
ConstTokenSlice::operator const int16_t&() const {
    return t->int16Vec_[i];
}
ConstTokenSlice::operator const int32_t&() const {
    return t->intVec_[i];
}
ConstTokenSlice::operator const uint32_t&() const {
    return t->uintVec_[i];
}
ConstTokenSlice::operator const float&() const {
    return t->floatVec_[i];
}
