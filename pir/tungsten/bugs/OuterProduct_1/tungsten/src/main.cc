
#include <iostream>
#include <fstream>
#include <istream>
#include "repl.h"
#include "DUT.h"
#include "cppgenutil.h"

using namespace std;

void printHelp() {
  fprintf(stderr, "Help for app: OuterProduct_1\n");
  fprintf(stderr, "  -- Args:    <No input args>\n");
  fprintf(stderr, "    -- Example: ./tungsten \n");
  exit(1);
}
int main(int argc, char **argv) {
  vector<string> *args = new vector<string>(argc-1);
  for (int i=1; i<argc; i++) {
    (*args)[i-1] = std::string(argv[i]);
    if (std::string(argv[i]) == "--help" | std::string(argv[i]) == "-h") {printHelp();}
  }
  vector<int32_t>* x160 = new vector<int32_t>(256);
  for (int b1 = 0; b1 < 256; b1++) {
    (*x160)[b1] = b1;
  }
  vector<int32_t>* x161 = new vector<int32_t>(256);
  for (int b3 = 0; b3 < 256; b3++) {
    (*x161)[b3] = b3;
  }
  x162 = malloc(sizeof(int32_t) * 256 + 64);
  x162 = (void *) (((uint64_t) x162 + 64 - 1) / 64 * 64);
  cout << "Allocate mem of size 256 at " << (int32_t*)x162 << endl;
  x163 = malloc(sizeof(int32_t) * 256 + 64);
  x163 = (void *) (((uint64_t) x163 + 64 - 1) / 64 * 64);
  cout << "Allocate mem of size 256 at " << (int32_t*)x163 << endl;
  x164 = malloc(sizeof(int32_t) * 256*256 + 64);
  x164 = (void *) (((uint64_t) x164 + 64 - 1) / 64 * 64);
  cout << "Allocate mem of size 256*256 at " << (int32_t*)x164 << endl;
  memcpy(x162, &(*x160)[0], (*x160).size() * sizeof(int32_t));
  memcpy(x163, &(*x161)[0], (*x161).size() * sizeof(int32_t));
  RunAccel();
  vector<int32_t>* x295 = new vector<int32_t>(65536);
  for (int b41 = 0; b41 < 65536; b41++) {
    int32_t x290 = b41 >> 8;
    int32_t x291 = (int32_t) ((b41 % 256 + 256) % 256);
    int32_t x292 = (*x160)[x290];
    int32_t x293 = (*x161)[x291];
    int32_t x294 = x292 * x293;
    (*x295)[b41] = x294;
  }
  vector<int32_t>* x297 = new vector<int32_t>((*x295).size());
  for (int b49 = 0; b49 < (*x295).size(); b49++) { 
    int32_t x296 = (*x295)[b49];
    (*x297)[b49] = (int32_t) x296;
  }
  int32_t x300;
  if ((*x297).size() > 0) { // Hack to handle reductions on things of length 0
    x300 = (*x297)[0];
  }
  else {
    x300 = 0;
  }
  for (int b52 = 1; b52 < (*x297).size(); b52++) {
    int32_t b53 = (*x297)[b52];
    int32_t b54 = x300;
    int32_t x299 = b53 + b54;
    x300 = x299;
  }
  vector<int32_t>* x301 = new vector<int32_t>(65536);
  memcpy(&(*x301)[0], x164, (*x301).size() * sizeof(int32_t));
  vector<int32_t>* x304 = new vector<int32_t>((*x301).size());
  for (int b60 = 0; b60 < (*x301).size(); b60++) { 
    int32_t x303 = (*x301)[b60];
    (*x304)[b60] = (int32_t) x303;
  }
  int32_t x307;
  if ((*x304).size() > 0) { // Hack to handle reductions on things of length 0
    x307 = (*x304)[0];
  }
  else {
    x307 = 0;
  }
  for (int b63 = 1; b63 < (*x304).size(); b63++) {
    int32_t b64 = (*x304)[b63];
    int32_t b65 = x307;
    int32_t x306 = b64 + b65;
    x307 = x306;
  }
  string x308 = std::to_string(x300);
  string x309 = (string("expected cksum: ") + x308);
  string x310 = (x309 + string("\n"));
  std::cout << x310;
  string x312 = std::to_string(x307);
  string x313 = (string("result cksum:   ") + x312);
  string x314 = (x313 + string("\n"));
  std::cout << x314;
  bool x316 = x307 == x300;
  string x317 = x316 ? string("true") : string("false");
  string x318 = (string("PASS: ") + x317);
  string x319 = (x318 + string(" (OuterProduct)"));
  string x320 = (x319 + string("\n"));
  std::cout << x320;
  string x322 = ("\n=================\n" + (string("OuterProduct.scala:83:11: Assertion failure") + "\n=================\n"));
  if (true) { ASSERT(x316, x322.c_str()); }
  cout << "Complete Simulation" << endl;
}
