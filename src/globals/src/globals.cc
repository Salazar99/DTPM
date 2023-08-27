#include "globals.hh"
#include <limits>
#include <thread>
namespace l3Constants {
size_t MAX_THREADS = std::thread::hardware_concurrency();
} // namespace l3Constants
namespace l2Constants {
size_t MAX_THREADS = std::thread::hardware_concurrency();
} // namespace l2Constants

namespace l1Constants {
size_t MAX_THREADS = std::thread::hardware_concurrency() / 2;
} // namespace l1Constants

namespace clc {
std::vector<std::string> traceFiles;
std::vector<std::string> faultyTraceFiles;
std::string includeAss="";
std::string configFile = "";
std::string parserType = "";
std::string clk = "";
std::string outputLang = "Spot";
bool genTemp;
size_t maxThreads = std::thread::hardware_concurrency();
bool findMinSubset = false;
bool dumpAssToFile = false;
bool keepVacAss = false;
std::string dumpVacAss = "";
size_t maxAss = std::numeric_limits<size_t>().max();
std::string dumpPath = "";
bool intMode = false;
bool splitLogic = false;
size_t vcdRecursive = true;
size_t vcdUnroll=0;
std::string selectedScope = "";
bool dontFillAss = false;
bool dontPrintAss = false;
bool silent = false;
bool wsilent = false;
bool isilent = false;
bool psilent = false;
std::string clsAlg = "kmeans";
bool dumpStat = false;
bool dontNormalize = false;
bool sampleByCon = false;
} // namespace clc

// harm stat
namespace hs {

size_t timeToMine_ms = 0;
size_t nAssertions = 0;
size_t nOfCovFaults = 0;
size_t nFaults = 0;
size_t nFaultCovSubset = 0;
size_t traceLength = 0;
std::string name = "";

} // namespace hs
