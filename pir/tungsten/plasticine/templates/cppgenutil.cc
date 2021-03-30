#include "cppgenutil.h"

using namespace std;

int roundUpDiv(int a, int b) {
  return (a + b - 1) / b;
}

std::ostream& operator<<(std::ostream& stream, std::tuple<uint64_t, bool, bool>& reqst) {
  stream << std::get<0>(reqst) << "[" << std::get<1>(reqst) << std::get<2>(reqst) << "]";
  return stream;
}

std::ostream& operator<<(std::ostream& stream, std::array<uint64_t,16>& arr) {
  stream << "[";
  for (auto& d : arr)
    stream << d << " ";
  stream << "]";
  return stream;
}
std::ostream& operator<<(std::ostream& stream, std::array<uint64_t,1>& arr) {
  stream << "[";
  for (auto& d : arr)
    stream << d << " ";
  stream << "]";
  return stream;
}
string string_plus(const string &str1, const string &str2) {
  return str1 + str2;
}

void ASSERT(bool test, const char* msg) {
  if (!test) {
    cout << "Assertion fail:" << msg << endl;
    assert(false);
  }
}

void ASSERT(bool test, const string msg) {
  if (!test) {
    cout << "Assertion fail:" << msg << endl;
    assert(false);
  }
}

void set_token_en(Token& token, bool en) {
  token.nvalid = en ? 1 : 0;
}

Token make_token(bool value) {
  Token token;
  token.type = TT_BOOL;
  token.bool_ = value;
  return token;
}

Token make_token(int value) {
  Token token;
  token.type = TT_INT;
  token.int_ = value;
  return token;
}

Token make_token(uint32_t value) {
  Token token;
  token.type = TT_UINT;
  token.uint_ = value;
  return token;
}

Token make_token(long value) {
  Token token;
  token.type = TT_LONG;
  token.long_ = value;
  return token;
}

Token make_token(float value) {
  Token token;
  token.type = TT_FLOAT;
  token.float_ = value;
  return token;
}

Token make_token(uint64_t value) {
  Token token;
  token.type = TT_UINT64;
  token.uint64_ = value;
  return token;
}

ofstream& get_trace(string path) {
  if (traces.find(path) == traces.end()) {
    traces[path] = ofstream(path, std::ios::out);
  }
  return traces[path];
}

void cpAt(Token& token, const Token& from, int i) {
  switch(token.type) {
    case TT_FLOATVEC:
      token.floatVec_[i] = (float) from.floatVec_[i];
      break;
    case TT_FLOAT:
      token.float_ = (float) from.float_;
      break;
    case TT_INTVEC:
      token.intVec_[i] = (int) from.intVec_[i];
      break;
    case TT_INT8VEC:
      token.int8Vec_[i] = (int) from.int8Vec_[i];
      break;
    case TT_INT16VEC:
      token.int16Vec_[i] = (int) from.int16Vec_[i];
      break;
    case TT_INT:
      token.int_ = (int) from.int_;
      break;
    case TT_LONGVEC:
      token.longVec_[i] = (long) from.longVec_[i];
      break;
    case TT_LONG:
      token.long_ = (long) from.long_;
      break;
    case TT_UINT:
      token.uint_ = (uint32_t) from.uint_;
      break;
    case TT_UINTVEC:
      token.uintVec_[i] = (uint32_t) from.uintVec_[i];
      break;
    case TT_UINT64:
      token.uint64_ = (uint64_t) from.uint64_;
      break;
    case TT_UINT64VEC:
      token.uint64Vec_[i] = (uint64_t) from.uint64Vec_[i];
      break;
    case TT_BOOL:
      token.bool_ = (bool) from.bool_;
      break;
    case TT_BOOLVEC:
      token.boolVec_[i] = (bool) from.boolVec_[i];
      break;
    default:
      throw runtime_error("Unspported type ");
  }
}

void trace(int data, string path) {
  auto& stream = get_trace(path);
  stream << data << endl;
  stream.flush();
}
template <>
int parse(string& token) {
  return stoi(token);
}
template <>
bool parse(string& token) {
  return !(token == "false" || token == "0" || token == "False");
}
template <>
float parse(string& token) {
  return stof(token);
}
template <>
double parse(string& token) {
  return stod(token);
}

string replace(std::string data, std::string toSearch, std::string replaceStr)
{
  //Get the first occurrence
  size_t pos = data.find(toSearch);

  // Repeat till end is reached
  while( pos != std::string::npos )
  {
    // Replace this occurrence of Sub String
    data.replace(pos, toSearch.size(), replaceStr);
    // Get the next occurrence from the current position
    pos =data.find(toSearch, pos + replaceStr.size());
  }
  return data;
}

template<>
int from_string<int>(string& token) {
	return stoi(token);
}

template<>
float from_string<float>(string& token) {
	return stof(token);
}

template<>
bool from_string<bool>(string& token) {
	return stoi(token);
}
