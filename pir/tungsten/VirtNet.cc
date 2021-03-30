#include "sparsity/templates/VirtNet.h"
#include "module.h"

vector<DelayNetBuild*> NetworkLinkManager::cbs {};
vector<NetworkLinkManager::LinkSpec> NetworkLinkManager::all_links {};
vector<NetworkLinkManager::NodeDest> NetworkLinkManager::node_dest_links {};
vector<NetworkLinkManager::NodeSrc> NetworkLinkManager::node_src_links {};
vector<NetworkLinkManager::NodeSpec> NetworkLinkManager::all_nodes {};
map<string,string> NetworkLinkManager::node_map {};
int NetworkLinkManager::next_spdram {1};

void AddNode(const string& _name, int type) {
  NetworkLinkManager::AddNode(_name, type);
}

int NetworkLinkManager::GetSpDRAMID() {
  return next_spdram++;
}

void NetworkLinkManager::AddLink(const string& from, 
    const vector<string>& to) {
  all_links.emplace_back(from, to);
}
NetworkLinkManager::NetworkLinkManager(const string& _name) : Module(_name) {}
void NetworkLinkManager::AddNode(const string& _name, int type) {
  all_nodes.emplace_back(_name, type);
}
void NetworkLinkManager::AddDestLink(const string& _name, const string& node){ 
  node_dest_links.emplace_back(_name, node);
  node_map[_name] = node;
  //cout << "Link dest " << _name << " to node " << node << endl;
}
void NetworkLinkManager::AddSrcLink(const string& _name, const string& node, int type){ 
  node_src_links.emplace_back(_name, node, type);
  node_map[_name] = node;
}
string NetworkLinkManager::GetNodes() {
  ostringstream ret;

  //for (auto m : cbs) {
  //m->BuildNet();
  //}
  ret << "Node,Type" << endl;
  for (auto n : all_nodes) {
    ret << n.name << ",";
    ret << n.type << endl;
  }
  return ret.str();
}

string NetworkLinkManager::GetLinks() {
  ostringstream ret;
  int max_link {0};
  for (auto l : all_links)
    if (l.to.size() > max_link)
      max_link = l.to.size();
  ret << "LinkID";
  for (int i=0; i<max_link; i++)
    ret << ",ToAddr";
  ret << endl;
  for (auto l : all_links) {
    ret << l.from;
    for (auto t : l.to) {
      //ret << "," << t  << "," << node_map[t];
      ret << "," << t;

    }
    ret << endl;
  }
  return ret.str();
}

string NetworkLinkManager::SrcLinks() {
  ostringstream ret;
  ret << "Src,CTX,Node,Type,Count" << endl;
  for (const auto& l : node_src_links) {
    assert(node_map[l.src] == l.node);
    ret << l.src << ",UNK_CTX," << l.node <<"," << (l.type?l.type:1) <<"," << 10000 << endl;
  }
  return ret.str();
}

string NetworkLinkManager::DestLinks() {
  ostringstream ret;
  ret << "Dest,Node" << endl;
  for (const auto& l : node_dest_links) {
    assert(node_map[l.dest] == l.node);
    ret << l.dest << "," << l.node << endl;
  }
  return ret.str();
}

string NetworkLinkManager::GetParameter(const string& key) {
  if (key == "links") {
    return NetworkLinkManager::GetLinks();
  } else if (key == "nodes") {
    return NetworkLinkManager::GetNodes();
  } else if (key == "src_link") {
    return NetworkLinkManager::SrcLinks();
  } else if (key == "dest_link") {
    return NetworkLinkManager::DestLinks();
  } else {
    throw module_error(this, "unknown key");
  }
}


VirtualNetworkLink::VirtualNetworkLink(const std::string& _name,
    net_pack nets,
    int _type,
    int count,
    bool _en_src,
    bool _en_dest) : 
  VirtualNetworkLink(_name, get<0>(nets), get<1>(nets), get<2>(nets), _type, count, _en_src, _en_dest) {
  }
VirtualNetworkLink::VirtualNetworkLink(const std::string& _name,
    NetworkInterface<Token, int, int> *_dynnet,
    NetworkInterface<Token, int, int> *_statnet,
    NetworkInterface<Token, int, int> *_idealnet,
    int _type,
    int count, 
    bool _en_src,
    bool _en_dest) :
  Module(_name),
  dynnet(_dynnet),
  statnet(_statnet),
  idealnet(_idealnet),
  type(_type),
  en_src(_en_src),
  en_dest(_en_dest),
  in("in",   _dynnet, _statnet, _idealnet) {
    
    if (en_src)
      AddChild(&in);

    if (en_dest) {
      for (int i=0; i<count; i++) {
        outs.push_back(new NetworkOutput("out"+to_string(i),
              _dynnet, _statnet, _idealnet));
        AddChild(outs.back());
        out_mod.push_back(NULL);
        out_str.push_back("");
      }
    }
    //NetworkLinkManager::RegisterCB(this);
  }

void VirtualNetworkLink::Disable() {
  assert(en);
  en = false;
}
void VirtualNetworkLink::Finalize() {
  if (en) BuildNet();
}
void VirtualNetworkLink::AddOuts(int new_max) {
  assert(en_dest);
  assert(new_max > outs.size());
  for (int i=outs.size(); i < new_max; i++) {
    outs.push_back(new NetworkOutput("out"+to_string(i),
          dynnet, statnet, idealnet));
    AddChild(outs.back());
    out_mod.push_back(NULL);
    out_str.push_back("");
  }
}

void VirtualNetworkLink::DisInput() {
  en_src = false;
}

void VirtualNetworkLink::DisOutput() {
  en_dest = false;
}

void VirtualNetworkLink::BuildNet() {
  from = in.path+in.name;
  vector<string> out_names;
  for (auto o : outs) {
    out_names.push_back(o->path+o->name);
  }
  if (en_src && en_dest) NetworkLinkManager::AddLink(from, out_names);
  if (en_src) {
    if (in_mod) {
      assert(in_str == "");
      NetworkLinkManager::AddSrcLink(from, in_mod->path+in_mod->name, type);
    } else { 
      if (in_str == "")
        throw module_error(this, "no input set");
      NetworkLinkManager::AddSrcLink(from, in_str, type);
    }
  }
  if (en_dest) {
    for (int i=0; i<outs.size(); i++) {
      if (out_mod[i]) {
        assert(out_str[i] == "");
        NetworkLinkManager::AddDestLink(outs[i]->path+outs[i]->name, out_mod[i]->path+out_mod[i]->name);
      } else {
        if (out_str[i] == "")
          throw module_error(this, "no output set for index " +to_string(i));
        NetworkLinkManager::AddDestLink(outs[i]->path+outs[i]->name, out_str[i]);
      }
    }
  }
}

CheckedReceive<Token>* VirtualNetworkLink::o(int i) {
  return outs.at(i);
}

const CheckedReceive<Token>* VirtualNetworkLink::o(int i) const {
  return outs.at(i);
}

void VirtualNetworkLink::RegIn(Module *_in_mod) {
  assert(!in_mod);
  in_mod = _in_mod;
}

void VirtualNetworkLink::RegIn(string _in_str) {
  assert(in_str == "");
  in_str = _in_str;
}

void VirtualNetworkLink::RegOut(Module *_out_mod, int i) {
  if (!(out_str[i] == "" && !out_mod[i]))
    throw module_error(this, "duplicate output registration");
  out_mod[i] = _out_mod;
}

void VirtualNetworkLink::RegOut(string _out_str, int i) {
  assert(out_str[i] == "" && !out_mod[i]);
  out_str[i] = _out_str;
}

void VirtualNetworkLink::RegisterInputModule(Module *_in_mod)  {
  assert(!in_mod);
  in_mod = _in_mod;
}

void VirtualNetworkLink::RegisterOutputModule(Module *_out_mod, int i)  {
  assert(!out_mod[i]);
  out_mod[i] = _out_mod;
}

// CheckedSend Interfaces 
void VirtualNetworkLink::Push(const Token& v) { in.Push(v);        }
bool VirtualNetworkLink::Ready() const        {  assert(en); return in.Ready(); }

// CheckedReceive Interfaces 
bool VirtualNetworkLink::Valid() const {
  assert(en);
  assert(outs.size() == 1); 
  return o(0)->Valid(); 
}
void VirtualNetworkLink::Pop() {
  assert(outs.size() == 1); 
  o(0)->Pop();
}
const Token& VirtualNetworkLink::Read() const {
  assert(outs.size() == 1); 
  return o(0)->Read();
}
