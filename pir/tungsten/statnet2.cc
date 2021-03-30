#include "statnet2.h"

#include <regex>

bool BufferPoint::_ready(void) const {
  if (!buf.size() || !out.size())
    return false;
  for (const auto& [p, c] : out)
    if (!c)
      return false;
  return true;
}

void BufferPoint::_send(void) {
  for (auto& [p, c] : out) {
    c--;
    parent->Packet(p, buf.front());
  }

  // cout << name << " send!" << endl;

  buf.pop_front();
  if (in.first)
    parent->Credit(in.first, in.second);
}

BufferPoint::BufferPoint(const string& _name, StaticNetwork2* _parent, int _max_depth) 
  : name(_name), parent(_parent), max_depth(_max_depth), in(nullptr, 0) {

}

void BufferPoint::SetIn(BufferPoint *p, int idx) {
  assert(!in.first);
  in = make_pair(p, idx);
}

int BufferPoint::AddOut(BufferPoint *p) {
  // Safety check to make sure the same buffer isn't added twice
  for (auto& [b, c] : out)
    assert(b != p);

  out.emplace_back(p, max_depth);
  return out.size() - 1;
}

void BufferPoint::Credit(int idx) {
  int pre = out[idx].second++;
  // if (!pre && _ready())
  if (_ready())
    _send();
}

// void BufferPoint::Packet(shared_ptr<Token>&& t) {
void BufferPoint::Packet(shared_ptr<Token> t) {
  buf.emplace_back(t);
  // if (buf.size() == 1 && _ready())
  if (_ready())
    _send();
}

shared_ptr<Token> BufferPoint::ReadPop(void) {
  auto ret = move(buf.front());
  buf.pop_front();

  if (in.first)
    parent->Credit(in.first, in.second);

  return ret;
}

void StaticNetwork2::AddLink(const string& l, bool output) {
  int flow         {-1};
  int source       {-1};
  int destination  {-1};
  std::regex route_regex("([fsd][0-9]+)");
  auto matches_begin = std::sregex_iterator(l.begin(), l.end(), route_regex);
  auto matches_end = std::sregex_iterator();

  for (auto i = matches_begin; i != matches_end; i++) {
    std::smatch match = *i;
    std::string match_str = match.str();
    int match_int = std::stoi(std::string(++match_str.begin(),
                                          match_str.end()));
    char match_char = match_str[0];
    if (match_char == 's') {
      if (source != -1)
        throw std::runtime_error("Attempt to set source twice ("+l+")");
      source = match_int;
    } else if (match_char == 'f') {
      if (flow != -1)
        throw std::runtime_error("Attempt to set flow twice ("+l+")");
      flow = match_int;
    } else if (match_char == 'd') {
      if (destination != -1)
        throw std::runtime_error("Attempt to set destination twice ("+l+")");
      destination = match_int;
    }
  }

  cout << "Add link flow: " << flow << " source: " << source << " destination: " << destination << " output: " << output << endl;
  links_orig.emplace_back(flow, source, destination, output);
}

void StaticNetwork2::BuildAll(void) {
  // Start by constructing buffers
  for (auto& [flow, source, destination, output] : links_orig) {
    auto src_str = "(" + to_string(flow) + "," + to_string(source) + ")";
    auto dst_str = "(" + to_string(flow) + "," + to_string(destination) + ")";
    if (output) {
      if (!bufs[make_pair(flow, source)])
        bufs[make_pair(flow, source)] = new BufferPoint(src_str, this, depth);
      assert(!outputs[make_pair(flow, destination)]);
      outputs[make_pair(flow, destination)] = new BufferPoint(dst_str, this, depth);
    } else {
      // Create buffers at the source and destination (if necessary)
      if (!bufs[make_pair(flow, source)])
        bufs[make_pair(flow, source)] = new BufferPoint(src_str, this, depth);
      if (!bufs[make_pair(flow, destination)])
        bufs[make_pair(flow, destination)] = new BufferPoint(dst_str, this, depth);
    }
  }

  // Now, look at each link and register the appropriate buffers
  for (auto& [flow, source, destination, output] : links_orig) {
    BufferPoint *from, *to;
    if (output) {
      cout << "Output link from (" << flow << "," << source << ") to (" << flow << "," << destination << ")" << endl;
      from = bufs.at(make_pair(flow, source));
      to = outputs.at(make_pair(flow, destination));
    } else {
      cout << "Link from (" << flow << "," << source << ") to (" << flow << "," << destination << ")" << endl;
      from = bufs.at(make_pair(flow, source));
      to = bufs.at(make_pair(flow, destination));
    }

    int dest_idx = from->AddOut(to);
    to->SetIn(from, dest_idx);
  }
}

StaticNetwork2::StaticNetwork2(const string& _name, int _delay, int _depth) 
  : Module(_name), delay(_delay), depth(_depth) {
}
 
void StaticNetwork2::Credit(BufferPoint *p, int idx) {
  assert(p);
  // cout << "register credit" << endl;
  creds_pend.emplace_back(cycle+delay, p, idx);
}

void StaticNetwork2::Packet(BufferPoint *p, shared_ptr<Token>& t) {
  assert(p && t);
  // cout << "register packet" << endl;
  packets_pend.emplace_back(cycle+delay, p, t);
}

void StaticNetwork2::Clock(void) {
  int creds_erase{0};
  int packets_erase{0};

  // for (auto [c, p, idx] : creds_pend) {
  for (int i = 0; i < creds_pend.size(); i++) {
    auto& [c, p, idx] = creds_pend[i];
    if (c > cycle)
      break;
    creds_erase++;
    // cout << "RETURN CREDIT" << endl;
    p->Credit(idx);
  }
  for (int i=0; i<creds_erase; i++)
    creds_pend.pop_front();
  // for (auto [c, p, t] : packets_pend) {
  for (int i = 0; i < packets_pend.size(); i++) {
    auto& [c, p, t] = packets_pend[i];
    if (c > cycle)
      break;
    packets_erase++;
    // cout << "FORWARD PACKET" << endl;
    // p->Packet(move(t));
    p->Packet(t);
  }
  for (int i=0; i<packets_erase; i++)
    packets_pend.pop_front();
  cycle++;
}

void StaticNetwork2::Eval(void) {
}

void StaticNetwork2::AppendParameter(const string& key, const string& val) {
  if (key == "link") {
    AddLink(val, false);
  } else if (key == "output") {
    AddLink(val, true);
  } else {
    throw module_error(this, "Unknown key "+key+" at "+name);
  }
}

void StaticNetwork2::Apply(const string& key, const vector<string>& vals) {
  if (key == "finalize") {
    BuildAll();
  } else {
    throw module_error(this, "Unknown key "+key+" at "+name);
  }
}

bool StaticNetwork2::Ready(int src, int vc, int flow) const {
  return bufs.at(make_pair(flow, src))->Ready();
}

void StaticNetwork2::Push(const Token& v, int src, int vc, int flow) {
  // cout << "Push input src: " << src << " vc: " << vc << endl;
  bufs[make_pair(flow, src)]->Packet(make_shared<Token>(v));
}

bool StaticNetwork2::Valid(int dst, int vc) const {
  // cout << "Poll output dst: " << dst << " vc: " << vc << endl;
  return outputs.at(make_pair(vc, dst))->Valid();
}

Token StaticNetwork2::Read(int dst, int vc) {
  return *outputs[make_pair(vc, dst)]->ReadPop();
}
