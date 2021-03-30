#include "module.h"

std::atomic<unsigned long int> Module::remaining;

Module::Module(std::initializer_list<Module*> _children,
    const std::string& _name) :
  name(_name), path("/"), writer(GetSession()) {
    for (auto c : _children)
      AddChild(c);
  }

Module::Module(const std::string& _name) :
  name(_name), path("/"), writer(GetSession()) {
  }

void Module::AddChild(Module *c, bool clock) {
  c->path = path+name+"/";
  if (clock)
    children.push_back(c);
  else
    name_children.push_back(c);
}

void Module::AddChildren(std::initializer_list<Module*> _children, bool clock) {
  for (auto* c : _children)
    AddChild(c, clock);
}

void Module::SetType(int _type) {
  type = _type;
}

// void Module::SetSession(binlog::Session& session) {
  // writer::~SessionWriter();
  // new (&writer) binlog::SessionWriter(session);
  // writer = binlog::SessionWriter(REPL::GetSession());
// }

void Module::Finalize() {
  if (type >= 0) {
    //NetworkLinkManager::AddNode(path+name, type);
    AddNode(path+name, type);
  }
  writer.setName(path+name);
}
void Module::SetParameter(const std::string& key, const std::string& val) {
  throw std::runtime_error(this->name + " has no key " + key);
}
void Module::AppendParameter(const std::string& key, const std::string& val) {
  throw std::runtime_error(this->name + " has no key " + key);
}
void Module::Apply(const std::string& cmd, const std::vector<std::string>& vals) {
  throw std::runtime_error(this->name + " has no command " + cmd);
}
std::string Module::GetParameter(const std::string& key) {
  throw std::runtime_error(this->name + " has no key " + key);
}
std::vector<Module*> Module::FindChildren(const std::deque<std::string>& search_path) {
  std::vector<Module*> ret;
  if (search_path.size() == 0) {
    // std::cout << "Leaf module, returning: " << this->path << name << std::endl;
    ret.push_back(this);
  } else {
    std::deque<std::string> rem;
    for (int i=1; i < search_path.size(); i++)
      rem.push_back(search_path[i]);
    // Enforce full matching
    std::regex r("^"+search_path[0]+"$");
    // std::cout << this->path << name << "search children for: " << ("^"+search_path[0]+"$") << std::endl;
    std::vector<Module*> to_search;
    std::copy(children.begin(), children.end(), std::back_inserter(to_search));
    std::copy(name_children.begin(), name_children.end(), std::back_inserter(to_search));
    for (auto c : to_search) {
      if (std::regex_search(c->name, r)) {
        auto tmp = c->FindChildren(rem);
        std::copy(tmp.begin(), tmp.end(), std::back_inserter(ret));
      }
    }
  }
  return ret;
}

Module* Module::FindChild(const std::string& ID) {
  for (Module *c : children) {
    if (c->name == ID)
      return c;
  }
  for (Module *c : name_children) {
    if (c->name == ID)
      return c;
  }
  throw std::runtime_error("Module " + name + " has no child named " + ID);
}
Module* Module::FindDescendent(const std::string& ID) {
  for (Module *c : children) {
    if (c->name == ID)
      return c;
    auto* d = c->FindDescendent(ID);
    if (d != NULL) return d;
  }
  for (Module *c : name_children) {
    if (c->name == ID)
      return c;
    auto* d = c->FindDescendent(ID);
    if (d != NULL) return d;
  }
  return NULL;
}
void Module::ClearChildren() {
  children.clear();
  name_children.clear();
}
std::vector<Module*> Module::BuildAll() {
  std::vector<Module*> ret;
  RefreshChildren();
  for (Module *c : children) {
    c->path = path+name+"/";
    for (Module *m : c->BuildAll()) {
      ret.push_back(m);
    }
  }
  for (Module *c : name_children) {
    c->path = path+name+"/";
    for (Module *m : c->BuildAll()) {
      ret.push_back(m);
    }
  }
  ret.push_back(this);
  return ret;
}
std::vector<Module*> Module::Build() {
  std::vector<Module*> ret;
  RefreshChildren();
  for (Module *c : children) {
    c->path = path+name+"/";
    for (Module *m : c->Build()) {
      ret.push_back(m);
    }
  }
  //for (Module *c : name_children) {
  //c->path = path+name+"/";
  // Continue to build name children: even if a module A is a name-child of
  // module B, module B may have true children [but probably shouldn't, for
  // performance purposes].
  //for (Module *m : c->Build()) {
  //std::cout << "Warning: module " << c->name << " is a name child of " << name << " but has child " << m->name << std::endl;
  //ret.push_back(m);
  //}
  //}
  ret.push_back(this);
  return ret;
}

void Module::Eval() {}
void Module::Log() {}
void Module::Log(std::ostream& log) { enLog = false; } // turn off logging unless overwritten
void Module::Clock() {}
void Module::RefreshChildren() {}
void Module::Expect(unsigned long int count) {
  remaining += count;
  local_remaining += count;
}
void Module::ResetExpect() {
  remaining -= local_remaining;
  local_remaining = 0;
}
void Module::Complete(unsigned long int count, bool force) {
  if (local_remaining < count && !force) {
    // assert(false);
    throw module_error(this, "completed more than expected");
  }
  remaining -= count;
  local_remaining -= count;
}
bool Module::Finished() {
  return !remaining;
}
void Module::SetInstrumentation() {
  instrumentation = true;
}
void Module::Active() {
  active = true;
}
void Module::Count() { // Called by repl every cycle
  if (instrumentation) {
    if (active) {
      active_cnt +=1;
      continue_inactive_cnt = 0;
    } else {
      inactive_cnt += 1;
      continue_inactive_cnt += 1;
    }
    active = false;
  }
}
void Module::DumpState(std::ostream& log) { // called by repl and override by submodules
  log << "\"active\":" << active_cnt;
  log << ",\"inactive\":" << inactive_cnt;
  log << ",\"cont_inactive\":" << continue_inactive_cnt;
}
