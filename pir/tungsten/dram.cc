#include "dram.h"
#include <iomanip>
#include <iostream>
#include <numeric>
#include <mutex>

#include "DDR4.h"
#include "DDR3.h"
#include "HBM.h"
#include "Statistics.h"
#include "binlog/binlog.hpp"

//#define DEBUG_DRAM

// TESTING

// ramulator has global state for the stats...
// what is wrong with people who write DRAM simulators...
static bool ramulatorStatsInit = false;

std::ostream& operator<<(std::ostream& stream, DRAMCommand& cmd){
  stream << "a:" << cmd.addr << " w:" << cmd.write;
  return stream;
}

std::ostream& operator<<(std::ostream& stream, SparseDRAMCommand& cmd){
  stream << "Addr: " ;
  for (int i=0; i<16; i++)
    stream << setw(10) << cmd.addr[i];
  stream << endl << "Data: ";
  for (int i=0; i<16; i++)
    stream << setw(10) << cmd.data[i];
  stream << endl;
  return stream;
}

DRAMController::DRAMController(const std::string& _name,
                               const std::string& _memfile,
                               std::initializer_list<CheckedReceive<DRAMCommand>*> _inputs,
                               std::initializer_list<CheckedSend<DRAMCommand>*> _outputs) :
  Module(_name), memfile(_memfile), inputs(_inputs), outputs(_outputs), tracename(_name) {
    robs.resize(inputs.size());
    assert(inputs.size() == outputs.size());
}

DRAMController::~DRAMController() {
  std::cout << "Destruct DRAM." << std::endl;
  mem->finish();
  Stats::statlist.printall();
}

void DRAMController::Finalize() {
}
void DRAMController::FinalizeClock() {
  assert(!mem);

  static map<string, function<ramulator::MemoryBase *(const ramulator::Config&, int)>> memory_type = {
    {"DDR4", &ramulator::MemoryFactory<ramulator::DDR4>::create},
    {"DDR3", &ramulator::MemoryFactory<ramulator::DDR3>::create},
    {"HBM", &ramulator::MemoryFactory<ramulator::HBM>::create}
  };

  configs = ramulator::Config(memfile);
  configs.set_core_num(1);
  configs.add("mapping", "defaultmapping");
  const string& std_name = configs["standard"];
  assert(memory_type.find(std_name) != memory_type.end() && "unrecognized standard name");
  mem = memory_type[std_name](configs, 64);
  if (!mem)
    throw module_error(this, "Unable to initialize DRAM controller");

  int simClkMhz = 1000;
  int memClkMhz = std::stoi(configs["clkMhz"]);
  int gcd = std::gcd(memClkMhz, simClkMhz);
  memTicks = memClkMhz / gcd;
  simTicks = simClkMhz / gcd;

  dram_ticks.fill(0);

  int memClkRem{memClkMhz};
  while (memClkRem) {
    float d = std::max(1000.0/memClkRem, 1.0);
    std::cout << "Tick sep: " << d << std::endl;
    for (float i=0; i<1000; i+=d) {
      if (memClkRem) {
        memClkRem--;
        dram_ticks[(int)i]++;
      }
    }
    if (d != 1)
      assert(memClkRem == 0);
  }

  std::cout << "memTicks: " << memTicks << " simTicks: " << simTicks << " gcd: " << gcd << std::endl;

  if (!ramulatorStatsInit) {
    Stats::statlist.output("ramulator.stats");
    ramulatorStatsInit = true;
  };
}

  void DRAMController::Fill(int32_t n) {
    for (auto& r : mem_data) 
      r.fill(n);
  }

void DRAMController::Add(CheckedReceive<DRAMCommand>* input, CheckedSend<DRAMCommand>* output) {
  input_served.push_back(0);
  inputs.push_back(input);
  outputs.push_back(output);

  robs.resize(inputs.size());
}

void DRAMController::AddSparse(CheckedReceive<SparseDRAMCommand>* input, CheckedSend<SparseDRAMCommand>* output) {
  sparse_inputs.push_back(input);
  sparse_outputs.push_back(output);

  sparse_robs.resize(sparse_inputs.size());
}

void DRAMController::AddMemRegion(uint64_t base, uint64_t size, const string& name) {
  mem_region new_r(base, size);
  for (auto& old_r : mem_data)
    if (old_r.intersects(new_r))
      throw module_error(this, "overlapping memory regions");
  if (name != "-")
    new_r.load(name);
  mem_data.push_back(new_r);
}

void DRAMController::DumpMemRegion(uint64_t base, uint64_t size, const string& name, bool uns) {
  for (auto& r : mem_data) {
    if (!r.maps(4*base))
      continue;
    r.dump(name, size, uns);
  }
}

void DRAMController::ReadCallback(const ramulator::Request &req) {
  //cout << "Read callback " << req.addr << endl;
  issued_read.erase(req.addr);
  sparse_issued_read.erase(req.addr>>bank_shift);
  served_read += 1;
  for (Rob& r : robs) {
    for (Rob_entry& e : r) {
      if (!std::get<2>(e) && std::get<0>(e) == req.addr) {
        std::get<1>(e) = true;
        read.Accum();
        if (enLog) *log << "Returned read " << std::get<0>(e) << std::endl;
      }
    }
  }

  for (SpRob& r : sparse_robs) {
    for (SpRob_entry& e : r) {
      if (e.write) 
        continue;
      for (int i=0; i<16; i++) {
        if (e.addrs[i]>>bank_shift == req.addr>>bank_shift)  {
          e.complete[i] = true;
          bool mapped{false};
          for (const auto& r : mem_data) {
            if (r.maps(e.addrs[i])) {
              e.data[i] = r.rd(e.addrs[i]);
              mapped = true;
            }
          }
          read.Accum();
#ifdef DEBUG_DRAM
          cout << "DRAM Returned read " << e.addrs[i] << " (" << e.data[i] << ")" << std::endl;
#endif
          if (!mapped)
            throw module_error(this, "unmapped dram address read: " + to_string(e.addrs[i]));
        }
      }
    }
  }
}

void DRAMController::WriteCallback(const ramulator::Request &req) {
  //cout << "Write callback " << req.addr << endl;
  served_write += 1;
  issued_write.erase(req.addr>>bank_shift);
  int n_ret {0};
  for (Rob& r : robs) {
    for (Rob_entry& e : r) {
      if (std::get<2>(e) && std::get<0>(e) == req.addr) {
        std::get<1>(e) = true;
        write.Accum();
        if (enLog) *log << "Returned write " << std::get<0>(e) << std::endl;
        n_ret++;

      }
    }
  }

  for (SpRob& r : sparse_robs) {
    for (SpRob_entry& e : r) {
      if (!e.write) 
        continue;
      for (int i=0; i<16; i++) {
        if (e.addrs[i]>>bank_shift == req.addr>>bank_shift)  {
          e.complete[i] = true;
          bool mapped{false};
          for (auto& r : mem_data) {
            if (r.maps(e.addrs[i])) {
              r.wr(e.addrs[i], e.data[i]);
              mapped=true;
            }
          }
          write.Accum();
#ifdef DEBUG_DRAM
          cout << "DRAM Returned write " << e.addrs[i] << ": " << e.data[i] << std::endl;
#endif
          if (!mapped)
            throw module_error(this, "unmapped dram address write");
          //if (enLog) *log << "Returned write " << req.addr << std::endl;
          //n_ret++;
        }
      }
    }
  }
  //assert(n_ret == 1);
}

ramulator::Request DRAMController::GetRequest(const DRAMCommand &cmd) {
  auto reqType = cmd.write ? ramulator::Request::Type::WRITE : ramulator::Request::Type::READ;
  std::function<void(ramulator::Request&)> readCallback = [=](ramulator::Request &r) {
    this->ReadCallback(r);
  };
  std::function<void(ramulator::Request&)> writeCallback = [=](ramulator::Request &r) {
    this->WriteCallback(r);
  };
  auto callback = cmd.write ? writeCallback : readCallback;
  return ramulator::Request(cmd.addr, reqType, callback);
}

void DRAMController::Clock() {
#ifdef GCD_TICK
  if (currentTick < memTicks) {
    mem->tick();
    Stats::curTick++;
  }
  currentTick++;
  if (currentTick % simTicks == 0) {
    // if the memory clock is faster than the simulator clock, make the extra ticks here
    for (int i = currentTick; i < memTicks; i++) {
      // std::cout << "memonly" << std::endl;
      mem->tick();
    }
    currentTick = 0;
  }
#else
  if (!mem)
    FinalizeClock();

  Stats::curTick++;
  for (int i=0; i < dram_ticks[currentTick]; i++) {
    // std::cout << "memtick" << std::endl;
    mem->tick();
  }
  currentTick++;
  currentTick %= 1000;
#endif
}

DRAMCommand DRAMController::SliceSparseCmd(const SparseDRAMCommand& cmd, int idx) {
  assert(idx >= 0);
  assert(idx < 16);
  DRAMCommand ret;
  ret.write = cmd.write;
  ret.addr = cmd.addr[idx];
  ret.data = cmd.data[idx];
  return ret;
}

void DRAMController::Apply(const string& cmd, const vector<string>& vals) {
  if (cmd == "dump_sparse") {
    for (int i=0; i<sparse_robs.size(); i++) {
      const auto& r = sparse_robs[i];
      cout << "Dump sparse ROB " << i << endl;
      for (const auto &e : r) {
        cout << "\t" << (e.write?"Write":" Read");
        for (int j=0; j<16; j++) {
          if (e.valid[j]) {
            cout << setw(8) << e.addrs[j] << (e.complete[j]?"☑":"❒");
          } else {
            cout << setw(9) << "";
          }
        }
        cout << endl;
        if (e.write) {
          cout << "\t Data";
          for (int j=0; j<16; j++) {
            if (e.valid[j]) {
              cout << setw(8) << e.data[j] << " ";
            } else {
              cout << setw(9) << "";
            }
          }
          cout << endl;
        }
      }
    }
  } else if (cmd == "list_regions") {
    for (const auto& r : mem_data)
      cout << "Base: " << r.base << " Size: " << r.size << endl;
  } else if (cmd == "dump" || cmd == "dump_u") {
    if (vals.size() != 3)
      throw module_error(this, "invalid number of arguments");
    bool uns{cmd == "dump_u"};
    uint64_t base = stoul(vals[0]);
    uint64_t size = stoul(vals[1]);
    DumpMemRegion(base, size, vals[2], uns);
  } else if (cmd == "region") {
    if (vals.size() != 3)
      throw module_error(this, "invalid number of arguments");
    uint64_t base = stoul(vals[0]);
    uint64_t size = stoul(vals[1]);
    AddMemRegion(base, size, vals[2]);
  } else if (cmd == "config") { 
    if (vals.size() != 1)
      throw module_error(this, "invalid number of arguments");
    memfile = vals[0];
    cout << "Set memory config to: " << memfile << endl;
    Finalize();
  } else {
    throw module_error(this, "unknown command");
  }
}

void DRAMController::Eval() {
  read.Step();
  write.Step();

  cycle++;

  vector<pair<uint64_t, int>> map;
  for (int i=0; i<inputs.size(); i++) {
    // map.emplace_back(robs[i].size(), i);
    map.emplace_back(input_served[i], i);
  }

  // TEST: shuffle 
  // shuffle(map.begin(), map.end(), rd);
  std::sort(map.begin(), map.end());

  vector<int> inputs_valid;
  vector<int> inputs_sent;

  // Iterate over each input, checking whether the transaction is valid.
  for (int ii=0; ii<inputs.size(); ii++) {
    if (inputs[ii]->Valid())
      inputs_valid.push_back(ii);
    int i = map[ii].second;
    if (inputs[i]->Valid() && robs[i].size() < robsize) {
      const DRAMCommand& c = inputs[i]->Read();
      auto req = GetRequest(c);
      if (!c.write && issued_read.count(c.addr))  {
        input_served[i] = cycle;
        inputs_sent.push_back(i);
        robs[i].push_back(std::make_tuple(c.addr, false, c.write));
        inputs[i]->Pop();
        if (enLog) *log << "Issue read in rob" << i << " " << c.addr << std::endl;
      } else if (mem->send(req)) {
        input_served[i] = cycle;
        inputs_sent.push_back(i);
        //std::cout << "Handle DRAM write transaction" << std::endl;
        robs[i].push_back(std::make_tuple(c.addr, false, c.write));
        if (!c.write) issued_read.insert(c.addr);
        /* Handle writes when they are sent to memory. */
        inputs[i]->Pop();
        if (enLog) {
          string tp = (c.write ? "write" : "read");
          *log << "Issue " + tp + " in rob" << i << " " << c.addr << std::endl;
        }
      }
    }
  }
  BINLOG_INFO_W(writer, "Valid: {} Sent: {}", inputs_valid, inputs_sent);
  for (int i=0; i<outputs.size(); i++) {
    if (outputs[i]->Ready() && !robs[i].empty()) {
      const Rob_entry& e = robs[i].front();
      if (std::get<1>(e)) {
        DRAMCommand c;
        c.addr = std::get<0>(e);
        c.write = std::get<2>(e);
        //std::cout << "Returning DRAM transaction" << std::endl;
        outputs[i]->Push(c);
        robs[i].pop_front();
      }
    }
  }


  // Same as above, but tweaked for sparse inputs
  int it_max{sparse_inputs.size()+last_sparse_in};
  for (int it=last_sparse_in; it<it_max; it++) {
    int i=it % sparse_inputs.size();
    bool last_rob_done {true};
    if (sparse_robs[i].size()) {
      auto& e = sparse_robs[i].back();
      for (int j=0; j<16; j++) {
        if (e.valid[j] && !e.sent[j]) {
          // We have a valid, unsent entry in the previous ROB back 
          last_sparse_in = i % sparse_inputs.size();
          last_rob_done = false;
#ifdef DEBUG_DRAM
          cout << "Retry DRAM transaction to " << e.addrs[j] << (e.write?" WRITE":" READ") << endl;
#endif

          DRAMCommand c;
          c.write = e.write;
          c.addr = e.addrs[j];
          c.data = e.data[j];
          auto req = GetRequest(c);
          if (!c.write && sparse_issued_read.count(c.addr>>bank_shift))  {
#ifdef DEBUG_DRAM
            //cout << "Coalesce read access" << endl;
#endif
            e.sent[j] = true;
          } else if (c.write && issued_write.count(c.addr>>bank_shift))  {
#ifdef DEBUG_DRAM
            //cout << "Coalesce write access" << endl;
#endif
            e.sent[j] = true;
          } else if (mem->send(req)) {
            if (!c.write) 
              sparse_issued_read.insert(c.addr>>bank_shift);
            else
              issued_write.insert(c.addr>>bank_shift);
            e.sent[j] = true;
          }
        }
      }
    }
    if (last_rob_done && sparse_inputs[i]->Valid() && sparse_robs[i].size() < robsize) {
      const SparseDRAMCommand& s_c = sparse_inputs[i]->Read();
      SpRob_entry e;
      e.write = s_c.write;
      for (int j=0; j<16; j++) {
        const DRAMCommand c = SliceSparseCmd(s_c, j);

        e.valid[j] = c.addr >= 0;
        e.addrs[j] = c.addr;
        e.data[j]  = c.data;

        if (c.addr < 0)
          continue;
        assert(e.addrs[j] >= 0);
#ifdef DEBUG_DRAM
        cout << "Process DRAM transaction to " << e.addrs[j] << (e.write?" WRITE":" READ") << endl;
#endif
        auto req = GetRequest(c);
        if (!c.write && sparse_issued_read.count(c.addr>>bank_shift))  {
          //cout << "Coalesce read access" << endl;
          e.sent[j] = true;
        } else if (c.write && issued_write.count(c.addr>>bank_shift))  {
          //cout << "Coalesce write access" << endl;
          e.sent[j] = true;
        } else if (mem->send(req)) {
          if (!c.write) 
            sparse_issued_read.insert(c.addr>>bank_shift);
          else
            issued_write.insert(c.addr>>bank_shift);
          e.sent[j] = true;
        }
      }
      last_sparse_in = i % sparse_inputs.size();
      sparse_robs[i].push_back(e);
      sparse_inputs[i]->Pop();
    }
  }

  for (int i=0; i<sparse_outputs.size(); i++) {
    if (sparse_outputs[i]->Ready() && !sparse_robs[i].empty()) {
      const SpRob_entry& e = sparse_robs[i].front();
      bool is_done {true};
      for (int j=0; j<16; j++)
        if (e.valid[j] && !e.complete[j])
          is_done = false;

      if (is_done) {
#ifdef DEBUG_DRAM
      cout << "Sparse output: " << i << " PUSH";
        cout <<endl;
#endif
        SparseDRAMCommand c;
        c.write = e.write;
        for (int j=0; j<16; j++) {
          if (e.valid[j])
            c.addr[j] = e.addrs[j];
          else
            c.addr[j] = -1;
          if (!e.write)
            c.data[j] = e.data[j];
          else
            c.data[j] = 0;
        }
        sparse_outputs[i]->Push(c);
        sparse_robs[i].pop_front();
      }
    }
  }
}

void DRAMController::Log(std::ostream& log) {
  for (int i = 0; i < inputs.size(); i++) {
    auto* in = inputs[i];
    auto* out = outputs[i];
    auto& rob = robs[i];
    auto v = in->Valid();
    log << " [v:" << v;
    log << " r:" << out->Ready();
    //if (v) {
      //const DRAMCommand& c = in->Read(); // cause read after pop
      //log << " a:" << mem->willAcceptTransaction(c.addr);
    //}
    log << " b:" << rob.size();
    if (rob.size() > 0) {
      log << " f:";
      auto& front = rob.front();
      log << (std::get<2>(front) ? "w" : "r" );
      log << "|" << std::get<0>(front);
    }
    log << "]";
  }
}

double DRAMController::GetAverageTotalPower() { // in mJ
  throw;
  //return mem->GetAverageTotalPower();
}

double DRAMController::GetAverageReadBW(double time) { // in GB/s
  return served_read * burstSize / 1e9 / time;
}

double DRAMController::GetAverageWriteBW(double time) { // in GB/s
  return served_write * burstSize / 1e9 / time;
}
const BandwidthStats& DRAMController::ReadBW() {
  return read;
}

const BandwidthStats& DRAMController::WriteBW() {
  return write;
}

void DRAMWriteAG::Eval() {
  if (addr->Valid() && data->Valid() && toCtrl->Ready()) {
    const Token& a = addr->Read();
    const Token& d = data->Read();
    
    DRAMCommand c;
    c.write = true;
    c.addr  = a.int_;
    //c.bytes = bytes;
    //c.dat   = d.intVec_;
    // std::cout << "Send DRAM write request to " << c.addr << std::endl;

    toCtrl->Push(c);
    addr->Pop();
    data->Pop();
  }
}
    
void DRAMReadAG::Eval() {
  if (addr->Valid() && en->Valid() && toCtrl->Ready()) {
    // std::cout << "Send DRAM read request" << std::endl;
    const Token& a = addr->Read();
    
    DRAMCommand c;
    c.write = false;
    c.addr  = a.int_;
    //c.bytes = bytes;

    toCtrl->Push(c);
    addr->Pop();
    en->Pop();
  }
}
