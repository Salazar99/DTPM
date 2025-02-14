#include "Miner.hh"
#include "globals.hh"
#include "harmIcon.hh"
#include "message.hh"
#include <chrono>
#include <filesystem>
#include <type_traits>

namespace harm {

Miner::ModulesConfig::ModulesConfig()
    : contextMiner(nullptr), propertyMiner(nullptr),
      propertyQualifier(nullptr) {
  // ntd
}

Miner::ModulesConfig::~ModulesConfig() {
  delete contextMiner;
  delete traceReader;
  delete propertyMiner;
  delete propertyQualifier;
}

Miner::Miner(ModulesConfig &configuration) : _config(configuration) {
  // ntd
}

void Miner::run() {

  messageInfo("Miner started...");

  std::vector<Context *> contexts;

  //1) Read the simulation traces
  messageErrorIf(_config.traceReader == nullptr,
                 "Trace reader module has not been set!");

  Trace *trace = _config.traceReader->readTrace();
  hs::traceLength = trace->getLength();

  //2) Read the contexts
  messageErrorIf(_config.contextMiner == nullptr,
                 "ContextMiner module has not been set!");
  _config.contextMiner->mineContexts(trace, contexts);

  messageInfo("Mining " + std::to_string(contexts.size()) + " contexts");

  for (Context *context : contexts) {
    messageWarningIf(context->_templates.empty(),
                     "No templates defined in context '" + context->_name +
                         "'");

    //remove "check" templates from the list, will be handled later
    std::vector<Template *> toCheck;
    context->_templates.erase(std::remove_if(context->_templates.begin(),
                                             context->_templates.end(),
                                             [&toCheck](Template *t) {
                                               if (t->_check) {
                                                 toCheck.push_back(t);
                                               }
                                               return t->_check;
                                             }),
                              context->_templates.end());

    messageErrorIf(_config.propertyMiner == nullptr,
                   "No propertyMiner module has been set");

    // time to mine
    std::chrono::steady_clock::time_point begin =
        std::chrono::steady_clock::now();

    //3) Mine temporal assertions
    _config.propertyMiner->mineProperties(*context, trace);

    // store the time to mine of this context
    std::chrono::steady_clock::time_point end =
        std::chrono::steady_clock::now();
    hs::timeToMine_ms +=
        std::chrono::duration_cast<std::chrono::milliseconds>(end - begin)
            .count();

    messageErrorIf(_config.propertyQualifier == nullptr,
                   "No propertyQualifier module has been set");

    //4) Qualify the mined temporal assertions (additionally print and dump)
    _config.propertyQualifier->qualify(*context, trace);

    // handle "check" templates
    for (Template *t : toCheck) {
      t->check();
    }
    delete context;
  }

  delete trace;

  printStats();
}

void Miner::printStats() {
  std::cout << "========================================="
            << "\n";
  if (hs::name != "") {
    std::cout << "Name: " << hs::name << "\n";
  }
  std::cout << "Time to mine: " << (double)hs::timeToMine_ms / 1000.f << "s"
            << "\n";
  std::cout << "Number of assertions: " << hs::nAssertions << "\n";
  std::cout << "Trace length: " << hs::traceLength << "\n";
  if (!clc::faultyTraceFiles.empty()) {
    std::cout << "Number of faults: " << clc::faultyTraceFiles.size() << "\n";
    std::cout << "Faults covered: " << hs::nOfCovFaults << " ("
              << ((double)hs::nOfCovFaults / (double)hs::nFaults) * 100.f
              << "%)"
                 "\n";
    std::cout << "Covering subset: " << hs::nFaultCovSubset << "\n";
  }
  std::cout << "========================================="
            << "\n";

  if (clc::dumpStat) {
    std::string filename = "stat_out_" + hs::name + ".csv";
    bool exists = std::filesystem::exists(filename);
    std::ofstream of(filename,
                     exists ? std::ofstream::app : std::ofstream::trunc);
    if (!exists) {
      of << "Name; Trace length;Time to mine (s);N assertions;N faults; Fault "
            "coverage (%);Cov. Subset;";
      of << "ClsAlg";
      of << "\n";
    }
    of << (hs::name == "" ? "?" : hs::name) << ";" << hs::traceLength << ";"
       << (double)hs::timeToMine_ms / 1000.f << ";" << hs::nAssertions << ";"
       << hs::nFaults << ";"
       << ((double)hs::nOfCovFaults / (double)hs::nFaults) * 100.f << ";"
       << hs::nFaultCovSubset << ";";

    of << clc::clsAlg;
    of << "\n";
    of.close();
  }
}

} // namespace harm
