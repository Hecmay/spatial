#include "network.h"

#include "binlog/binlog.hpp"

NetworkInput::NetworkInput(const std::string& _name,
    NetworkInterface<Token, int, int> *_dynnet,
    NetworkInterface<Token, int, int> *_statnet,
    NetworkInterface<Token, int, int> *_idealnet
    ) :
  Module(_name), dynnet(_dynnet), statnet(_statnet), idealnet(_idealnet), sent("sent"s) {
    SetInstrumentation();
  }
NetworkInput::NetworkInput(const std::string& _name,
    NetworkInterface<Token, int, int> *_dynnet,
    NetworkInterface<Token, int, int> *_statnet
    ) : NetworkInput(_name, _dynnet,_statnet,NULL) {}
void NetworkInput::SetParameter(const std::string& key, const std::string& val) {
  if (key == "addr") {
    addr = std::stoi(val);
  } else if (key == "flow") {
    flow = std::stoi(val);
  } else if (key == "vc") {
    vc = std::stoi(val);
  } else if (key == "net") {
    if (val == "static") {
      net = statnet;
    } else if (val == "dynamic") {
      net = dynnet;
    } else if (val == "ideal") {
      net = idealnet;
      if (idealnet == NULL)
        throw module_error(this, "haven't pass in ideal network!");
    } else {
      throw module_error(this, "unknown network type: " + val);
    }
  } else {
    throw std::runtime_error("Unknown key "+key+" at "+name);
  }
}
void NetworkInput::RefreshChildren() {
  AddChild(&sent, false);
}
void NetworkInput::Eval() {
  //if (this_cyc_starve)
  //  vec_starve++;
  this_cyc_starve=false;
  sent.Eval();
}
void NetworkInput::Clock() {
  sent.Clock();
}
bool NetworkInput::Ready() const {
  if (addr < 0 || flow < 0 || !net) {
    throw module_error(this, "Underinitialized network interface " + to_string(addr) + " " + to_string(flow));
  }
#ifdef DEBUG_NETWORK
  cout << "NI " << path << name << " ready:" << net->Ready(addr,vc,flow)<< endl;
#endif
  if (!net->Ready(addr, vc, flow)) {
    this_cyc_starve=true;
    return false;
  } else {
    return true;
  }
  //return net->Ready(addr, vc, flow);
}
void NetworkInput::Push(shared_ptr<Token> v) {
#ifdef DEBUG_NETWORK
  cout << "NI " << path << name << " push" << endl;
#endif
  if (enLog) *log << *v << std::endl;
  if (!net->Ready(addr, vc, flow))
    throw module_error(this, "push to backpressured interface");
  //run++;
  net->Push(v, addr, vc, flow);
  sent.Inc();
  Active();
}
void NetworkInput::Push(const Token& v) {
#ifdef DEBUG_NETWORK
  cout << "NI " << path << name << " push" << endl;
#endif
  // if (enLog) *log << v << std::endl;
  if (enLog) BINLOG_INFO_W(writer, "{}", v);
  if (!net->Ready(addr, vc, flow))
    throw module_error(this, "push to backpressured interface");
  //run++;
  net->Push(v, addr, vc, flow);
  sent.Inc();
  Active();
}
void NetworkInput::Log(std::ostream& log) {
  sent.Log();
  //log << " r:" << Ready() << " ";
  //log << "credit:" << net->GetCredit(addr, vc, flow) << " ";
  //net->LogSrc(log, addr, flow);
}
void NetworkInput::DumpState(std::ostream& log) {
  Module::DumpState(log);
  if (addr < 0 || flow < 0 || !net) {
    //log << "UNINIT";
  } else {
    log << ",\"ready\":" << Ready();
  }
}


NetworkOutput::NetworkOutput(const std::string& _name,
    NetworkInterface<Token, int, int> *_dynnet,
    NetworkInterface<Token, int, int> *_statnet,
    NetworkInterface<Token, int, int> *_idealnet
    ) :
  Module(_name), dynnet(_dynnet), statnet(_statnet), idealnet(_idealnet), received("received"s) {
    SetInstrumentation();
  }
NetworkOutput::NetworkOutput(const std::string& _name,
    NetworkInterface<Token, int, int> *_dynnet,
    NetworkInterface<Token, int, int> *_statnet) :
  NetworkOutput(_name,_dynnet,_statnet,NULL) {
    SetInstrumentation();
  }
void NetworkOutput::SetParameter(const std::string& key, const std::string& val) {
  if (key == "addr") {
    addr = std::stoi(val);
  } else if (key == "flow") {
    flow = std::stoi(val);
  } else if (key == "vc") {
    vc = std::stoi(val);
  } else if (key == "net") {
    if (val == "static") {
      net = statnet;
    } else if (val == "dynamic") {
      net = dynnet;
    } else if (val == "ideal") {
      net = idealnet;
      if (idealnet == NULL)
        throw module_error(this, "haven't pass in ideal network!");
    } else {
      throw module_error(this, "unknown network type: " + val);
    }
  } else {
    throw std::runtime_error("Unknown key "+key+" at "+name);
  }
}
void NetworkOutput::RefreshChildren() {
  AddChild(&received, false);
}
void NetworkOutput::Eval() {
  received.Eval();
}
void NetworkOutput::Clock() {
  received.Clock();
}
bool NetworkOutput::Valid() const {
  if (addr < 0 || flow < 0 || !net) {
    throw module_error(this, "Underinitialized network interface " + to_string(addr) + " " + to_string(flow));
  }
#ifdef DEBUG_NETWORK
  if (!dat && !net->Valid(addr, vc))
    cout << "NO " << path << name << " not valid" << endl;
#endif
  return dat || net->Valid(addr, vc);
}
void NetworkOutput::Pop() {
#ifdef DEBUG_NETWORK
  cout << "NO " << path << name << " pop" << endl;
#endif
  if (!dat.has_value()) {
    dat = net->Read(addr, vc);
    received.Inc();
  }
  if (!dat.has_value())
    throw std::runtime_error("Double pop from network output: " + name);
  // if (enLog) *log << Read() << endl;
  if (enLog) BINLOG_INFO_W(writer, "{}", Read());
  dat = std::nullopt;
  Active();
}
const Token& NetworkOutput::Read() const {
#ifdef DEBUG_NETWORK
  cout << "NO " << path << name << " read" << endl;
#endif
  if (!dat.has_value()) {
    dat = net->Read(addr, vc);
    received.Inc();
  }
  return dat.value();
}
void NetworkOutput::Log(std::ostream& log) {
  //bool v = Valid();
  //log << " v:" << Valid() << " ";
  //log << " d:";
  //net->LogDst(log, addr, vc);
}
void NetworkOutput::DumpState(std::ostream& log) {
  Module::DumpState(log);
  if (addr < 0 || flow < 0 || !net) {
    //log << "UNINIT";
  } else {
    log << ",\"valid\":" << Valid();
  }
}
