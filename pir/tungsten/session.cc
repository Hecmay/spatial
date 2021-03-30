#include "session.h"

binlog::Session& GetSession() {
  static binlog::Session* s = new binlog::Session();
  return *s;
}

