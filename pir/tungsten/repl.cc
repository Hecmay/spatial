#include <string>
#include <cctype>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <fstream>
#include <fnmatch.h>
#include <numeric>
#include <filesystem>
#include <typeinfo>

#include "unistd.h"

#include "dram.h"
#include "repl.h"
#include "cppgenutil.h"
#include "session.h"
// #include "pstl/algorithm"
// #include "pstl/execution"
#include "simtime.h"

using namespace std;

// uint64_t REPL::cycle{0};
// binlog::Session REPL::session;

REPL::REPL(Module *_DUT, std::ostream& _out) : DUT(_DUT), out(_out), chunked(1), logfile("tungsten.blog", std::ofstream::out|std::ofstream::binary) {
  init = std::chrono::steady_clock::now();
  Cmd_build({});
  for (Module *m : all) { 
    m->Finalize();
    // m->SetSession(session);
  }
}

void REPL::Run(std::istream& in, bool file, const std::string& prompt) {
  out << "Tungsten REPL started, commit " << GITCOMMIT << std::endl;

  std::string buf;

  while (1) {
    if (!quiet) out << prompt;
    std::getline(in, buf);
    if (!in)
      break;
    if (buf.length() == 0)
      continue;
    if (file & !quiet)
      out << buf << std::endl;
    Command(buf);
  }
  auto done = std::chrono::steady_clock::now();
  chrono::duration<double> secs = done - init;
  out << "Elapsed seconds: " << secs.count() << endl;
  out << "Exiting... 😊" << std::endl;
}

// void REPL::do_consume(std::ostream& _l, volatile int& done) {
void REPL::do_consume() {
  // cout << "I'm the consume manager!" << endl;
  while (!done) {
    GetSession().consume(logfile);
    sleep(1);
  }
  // Wait a second and do one final consumption.
  sleep(1);
  GetSession().consume(logfile);
  cout << endl;
}

long REPL::RunAll(unsigned long int timeout) {
  done = 0;
  // std::thread consume_manager(&REPL::do_consume, logfile, done);
  std::thread consume_manager(&REPL::do_consume, this);
  for (int i=0; i < clocked_all.size(); i++) {
    instrumentation |= clocked_all[i]->instrumentation;
  }
  last_tick = chrono::steady_clock::now();
  while (!deadlock && !DUT->Finished() && (timeout < 0 || cycle < timeout)) { 
    try {
      Tick();
    }
    catch (...) {
      done = 1;
      consume_manager.join();
      throw;      
    }
  }
  out << "Simulation complete at cycle " << cycle;
  if (deadlock) out << " \033[0;31m(DEADLOCK)\033[0m";
  out << std::endl;
  for (Module* m: all) {
    if (m->flog.is_open()) { m->flog.close(); }
  }
  for (auto& [name, trace]: traces)
    trace.close();
  done = 1;
  consume_manager.join();
  return cycle;
}

void REPL::Chunk(int size) {
  int this_chunk = 0;
  for (Module *m : all) {
    ++this_chunk;
    chunked.back().push_back(m);
    if (this_chunk == size)
      chunked.push_back({});
  }
}

void REPL::Command(const std::string& buf) {
  // Start by tokenizing the string (whitespace delimited)
  std::vector<std::string> tokens(1);
  for (char c : buf) {
    if (std::isspace(c) && tokens.back().size())
      tokens.push_back({});
    else if (!std::isspace(c))
      tokens.back().push_back(c);
  }
  if (!tokens.back().size())
    tokens.pop_back();
  for (const auto& c : cmds) {
    if (std::string(c.name) == tokens[0]) {
      try {
        (this->*(c.function))(tokens);
      } catch (std::exception& e) {
        out << "\033[0;31mCaught exception on cycle " << cycle << ": " << e.what() << "\033[0m" << std::endl;
      }
      return;
    }
  }
  out << "Command not found: " << tokens[0] << std::endl;
}

void REPL::Cmd_tick(const std::vector<std::string>& args) {
  if (args[0] == "step") {
    if (args.size() != 2)
      throw std::runtime_error("Invalid argument count!");

    RunAll(std::stoul(args[1]));
  } else if (args[0] == "stepall") {
    if (args.size() != 1)
      throw std::runtime_error("Invalid argument count!");
    RunAll(-1);
  } else {
    throw std::runtime_error("Invalid command!");
  }
}

Module* REPL::FindModule(const char* pattern) {
  string p(pattern);
  for (Module* m: all) {
    if (m->name == p) // wildcard match. 0 if matched
      return m;
  }
  throw std::runtime_error("No module match pattern: " + p);
}

void REPL::Cmd_get(const std::vector<std::string>& args) {
  if (args.size() != 3 && args.size() != 4)
    throw std::runtime_error("Invalid argument count!");

  std::deque<std::string> path(1);
  for (char c : args[1]) {
    if (c == '/' && path.back().size())
      path.push_back({});
    else if (c != '/')
      path.back().push_back(c);
  }

  if (!path.back().size())
    path.pop_back();

  if (path.front() == DUT->name)
    path.pop_front();
  Module *cur = DUT;
  for (const auto& p : path)
    cur = cur->FindChild(p);

  if (args.size() == 3)  {
    out << cur->GetParameter(args[2]) << std::endl; 
  } else {
    ofstream file(args[3]);
    file << cur->GetParameter(args[2]) << std::endl; 
    file.close();
  }
}

void REPL::Cmd_build(const std::vector<std::string>& args) {
  all = DUT->BuildAll(); // TODO: new modules should also be finalized

#if 0
  sort(all.begin(), 
       all.end(), 
       [](const Module *a, const Module *b) {
             return typeid(*a).hash_code() < typeid(*b).hash_code();
       });

#endif
  clocked_all = DUT->Build();

  sort(clocked_all.begin(), 
       clocked_all.end(), 
       [](const Module *a, const Module *b) {
             return typeid(*a).hash_code() < typeid(*b).hash_code();
       });
  out << "Run simulation with with " << clocked_all.size() << " children"<<std::endl;
  Chunk();
}

void REPL::Cmd_set(const std::vector<std::string>& args) {
  if (args.size() != 4 && args[0] != "apply")
    throw std::runtime_error("Invalid argument count!");

  //cout << "Applying command: ";
   //for (auto& a : args) 
      //cout << a << " ";
   //cout << endl;

  std::deque<std::string> path(1);
  for (char c : args[1]) {
    if (c == '/' && path.back().size())
      path.push_back({});
    else if (c != '/')
      path.back().push_back(c);
  }

  if (!path.back().size())
    path.pop_back();

  if (path.front() == DUT->name)
    path.pop_front();
  Module *cur = DUT;
  vector<Module*> mods;
  mods = DUT->FindChildren(path);
  // for (const auto& p : path)
    // cur = cur->FindChild(p);

  for (auto cur : mods) {
    if (args[0] == "set") {
      cur->SetParameter(args[2], args[3]);
    } else if (args[0] == "push") {
      cur->AppendParameter(args[2], args[3]);
    } else if (args[0] == "apply") {
      vector<string> tmp;
      for (int i=3; i<args.size(); i++) {
        tmp.emplace_back(args[i]);
      }
      cur->Apply(args[2], tmp);
    } else {
      throw std::runtime_error("Invalid argument count!");
    }
  }
}

void REPL::Cmd_exit(const std::vector<std::string>& args) {
  if (args.size() != 1)
    throw std::runtime_error("Invalid argument count!");

  std::exit(0);
}

void REPL::Cmd_source(const std::vector<std::string>& args) {
  if (args.size() != 2)
    throw std::runtime_error("Invalid argument count!");

  if (args[1] != "--"s) {
    std::ifstream input(args[1]);
    if (!input)
      throw std::runtime_error("Could not open file! " + args[1]);
    Run(input, true, args[1] + "> "s);
  } else {
    Run(std::cin);
  }
}

// Set dump module states to file
void REPL::Cmd_dumpstate(const std::vector<std::string>& args) {
  if (args.size() != 2)
    throw std::runtime_error("Invalid argument for dump path!");
  std::ofstream state(args[1], std::ofstream::out);
  state << "{" << std::endl;
  state << "\"cycle\":" << cycle << "," << std::endl;
  state << "\"deadlock\":" << deadlock << "," << std::endl;
  state << "\"modules\":{" << std::endl;
  int j = 0;
  for (auto* m: all) {
    if (m->instrumentation) {
      if (j != 0) state << "," << std::endl;
      state << "\"" << m->name << "\": {";
      m->DumpState(state);
      state << "}";
      j += 1;
    }
  }
  state << "}" << std::endl;
  state << "}" << std::endl;
}

// Dump DRAM stats to to file
void REPL::Cmd_printstat(const std::vector<std::string>& args) {
  for (auto m : all) {
    auto dram = dynamic_cast<DRAMController *>(m);
    if (dram) {
      //double W = dram->GetAverageTotalPower();
      //out << "Average DRAM Power: " << W << "W" << endl;
      double time = cycle * 1.0 / 1e9;
      double rbw = dram->GetAverageReadBW(time);
      double wbw = dram->GetAverageWriteBW(time);
      out << "Average " << dram->name << " Read Bandwidth: " << rbw << " GB/s" << endl;
      out << "Average " << dram->name << " Write Bandwidth: " << wbw << " GB/s" << endl;
    }
  }
}

void REPL::LogAll() {
  if ((cycle % logfreq == 0) && (cycle >= logstart)) {
#if 0
    for (int i=0; i < all.size(); i++) {
      if (!all[i]->enable) continue;
      all[i]->Log();
      if (all[i]->enLog) {
        auto& log = *all[i]->log;
        std::stringstream logStream;
        all[i]->Log(logStream);
        auto logString = logStream.str();
        if (!logString.empty()) {
          log << std::setw(8) << "#" + std::to_string(cycle) << " ";
          log << logString << std::endl;
          log.flush();
        }
      }
    }
#else
    for (auto m : logged_all) {
      if (!m->enable) continue;
      assert(m->enLog);

      auto& log = *m->log;
      std::stringstream logStream;
      m->Log(logStream);
      auto logString = logStream.str();
      if (!logString.empty()) {
        log << std::setw(8) << "#" + std::to_string(cycle) << " ";
        log << logString << std::endl;
        log.flush();
      }
    }
#endif
  }
}

void REPL::Tick() {
  // std::out << "Start cycle =======================" << std::endl;
  if (parallel) {
    // for_each(std::execution::par_unseq, chunked.begin(), chunked.end(),
        // [](std::vector<Module*> c) { for (Module *m : c) m->Eval(); });
    // for_each(std::execution::par_unseq, chunked.begin(), chunked.end(),
        // [](std::vector<Module*> c) { for (Module *m : c) m->Log(); });
    // for_each(std::execution::par_unseq, chunked.begin(), chunked.end(),
        // [](std::vector<Module*> c) { for (Module *m : c) m->Clock(); });
  } else {
    for (auto* m : clocked_all)
      if (m->enable) m->Eval();
    if (do_log) LogAll();
    deadlock = instrumentation;
    for (auto* m : clocked_all) {
      if (!m->enable) continue;
      m->Clock();
      m->Count();
      if (m->instrumentation) deadlock &= (m->continue_inactive_cnt > 3000);
#if 0
      if (m->instrumentation) {
        if (m->continue_inactive_cnt > 300) {
          cout << "Module inactive: " << m->path << m->name << endl;
        } else {
          cout << "Module Active: " << m->path << m->name << endl;
        }
      }
#endif
    }
    //for_each(all.begin(), all.end(), [](Module *m) { m->Eval(); });
    //for_each(all.begin(), all.end(), [](Module *m) { m->Log(); });
    //for_each(all.begin(), all.end(), [](Module *m) { m->Clock(); });
  }
  if (printfreq > 0 && !(cycle % printfreq)) { 
    out << setw(10) << cycle;
    auto cur_tick = chrono::steady_clock::now();
    chrono::duration<double> secs = cur_tick - last_tick;
    auto hertz = printfreq / secs.count();
    // out << setw(15) << secs.count();
    if (cycle)
      out << setw(15) << (int)hertz << " Hz";
    out << endl;
    last_tick = cur_tick;
  }
  // session.consume(logfile);
  // GetSession().consume(logfile);
  cycle += 1;
  SimTime::Cycle() = cycle;
}

void REPL::Cmd_log2files(const std::vector<std::string>& args) {
  std::ofstream dummy;
  if (!std::filesystem::exists("logs")) {
    std::filesystem::create_directory("logs");
  }
  for (Module* m: all) {
    m->Log(dummy); // Turn off enLog if the module doesn't have concrete implementation of Log
    if (m->enable && m->enLog) {
      std::string path = "logs/"+m->name+".log";
      m->flog.open(path, std::ofstream::out);
      m->log = &m->flog; // point to the file log
    }
  }
}
// Turn log on for specified module names. If none specified, turning on for all modules.
void REPL::Cmd_logon(const std::vector<std::string>& args) {
  do_log = true;
  for (Module* m: all) {
    if (args.size() == 1) {
      cout << "Log on module: " << m->name << endl;
      m->LogOn();
      m->enLog = true;
    } else {
      for (const std::string& arg : args) {
        //if (arg == m->name) 
        if (!fnmatch(arg.c_str(), m->name.c_str(), 0)) { // wildcard match. 0 if matched
          cout << "Log on module: " << m->name << endl;
          m->LogOn();
          m->enLog = true;
        }
      }
    }
  }
  regen_log();
}
// Turn log on for specified module names. If none specified, turning on for all modules.
void REPL::Cmd_logfile(const std::vector<std::string>& args) {
  assert(args.size() == 1);
  logfile = std::ofstream(args[0], std::ofstream::out|std::ofstream::binary);
  assert(logfile);
}

void REPL::regen_log(void) {
  logged_all.clear();
  for (Module *m : all) {
    if (m->enLog) {
      logged_all.push_back(m);
    }
  }
}

// Turn log off for specified module names. If none specified, turning on for all modules.
void REPL::Cmd_logoff(const std::vector<std::string>& args) {
  do_log = false;
  for (Module* m: all) {
    if (args.size() == 1) {
      m->LogOff();
      m->enLog = false;
    } else {
      for (const std::string& arg : args) {
        if (arg == m->name)  {
          m->LogOff();
          m->enLog = false;
        }
      }
    }
  }
  regen_log();
}

void REPL::Cmd_quiet(const std::vector<std::string>& args) {
  quiet = true;
}
void REPL::Cmd_verbose(const std::vector<std::string>& args) {
  quiet = false;
}

void toggle_module(Module* m, bool enable, std::set<Module*>& disabled) {
  cout << "Toggle module: " << m->name << endl;
  m->enable = enable;
  if (!enable) {
    m->enLog = false;
    disabled.insert(m);
  }
  for (auto* c: m->children) {
    toggle_module(c, enable, disabled);
  }
}
// Turn off evaluation of module and all its submodules for specified module names.
void REPL::Cmd_disable(const std::vector<std::string>& args) {
  for (Module* m: all) {
    for (const std::string& arg : args)
      if (!fnmatch(arg.c_str(), m->name.c_str(), 0)) { // wildcard match. 0 if matched
        toggle_module(m, false, disabled);
      }
  }
  vector<Module*> copy = all;
  all.clear();
  for (Module* m: copy) {
    if (!disabled.count(m)) {
      all.push_back(m);
    }
  }
}
// Turn off evaluation of module and all its submodules for specified module names.
void REPL::Cmd_printchildren(const std::vector<std::string>& args) {
  if (args[0] == "printchildren") {
    for (Module* m: all)
      out << "\t" << m->path << "/" << m->name << "\t" << typeid(*m).name() << endl;
  } else if (args[0] == "printclocked") {
    for (Module* m: clocked_all)
      out << "\t" << m->path << "/" << m->name << "\t" << typeid(*m).name() << endl;
    out << clocked_all.size() << " children" << endl;
  } else {
    throw std::runtime_error("Invalid command!");
  }
}
void REPL::Cmd_stallstarve(const std::vector<std::string>& args) {
  vector<string> strings;
  for (Module* m: clocked_all) {
    ostringstream tmp;
    if (m->vec_stall + m->vec_starve+m->scal_stall+m->scal_starve+m->run == 0)
      continue;
    tmp << "\t" << setw(50) << m->path+m->name << "\t";
    tmp << "sStall: " << setw(5) << setprecision(3) <<fixed<< 1.0*m->scal_stall/cycle; 
    tmp << " sStarve: " << setw(5) << setprecision(3) <<fixed<< 1.0*m->scal_starve/cycle; 
    tmp << " vStall: " << setw(5) << setprecision(3) <<fixed<< 1.0*m->vec_stall/cycle; 
    tmp << " vStarve: " << setw(5) << setprecision(3) <<fixed<< 1.0*m->vec_starve/cycle; 
    tmp << " Run: " << setw(5) << setprecision(3) <<fixed<< 1.0*m->run/cycle; 
    strings.push_back(tmp.str());
  }
  sort(strings.begin(), strings.end());
  for (auto s : strings)
    out << s << endl;
}
// Set log frequency to every args[1] cycles.
void REPL::Cmd_logevery(const std::vector<std::string>& args) {
  if (args.size() != 2)
    throw std::runtime_error("Invalid argument count!");
  logfreq = std::stoi(args[1]);
}
// Set cycle printing frequency to every args[1] cycles.
void REPL::Cmd_printevery(const std::vector<std::string>& args) {
  if (args.size() != 2)
    throw std::runtime_error("Invalid argument count!");
  printfreq = std::stoi(args[1]);
}
// Start logging from args[1] cycle.
void REPL::Cmd_logfrom(const std::vector<std::string>& args) {
  if (args.size() != 2)
    throw std::runtime_error("Invalid argument count!");
  logstart = std::stoi(args[1]);
}
