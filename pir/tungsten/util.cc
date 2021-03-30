#include <util.h>

string rep_str_loop(string name, int i, int j, int k) {
  regex I_re {"I"};
  regex J_re {"J"};
  regex K_re {"K"};

  name = regex_replace(name, I_re, to_string(i));
  name = regex_replace(name, J_re, to_string(j));
  name = regex_replace(name, K_re, to_string(k));

  return name;
}
