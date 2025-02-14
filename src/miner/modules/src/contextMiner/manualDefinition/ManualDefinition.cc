#include "ManualDefinition.hh"
#include "Assertion.hh"
#include "Context.hh"
#include "ProgressBar.hpp"
#include "Template.hh"
#include "Trace.hh"
#include "classes/atom/Atom.hh"
#include "dtLimitsParser.hh"
#include "globals.hh"
#include "metricParser.hh"
#include "metricParsingUtils.hh"
#include "minerUtils.hh"
#include "propositionParser.hh"
#include "templateParser.hh"

#include <algorithm>
#include <cassert>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace harm {

using namespace rapidxml;
ManualDefinition::ManualDefinition(std::string &configFile)
    : ContextMiner(configFile) {}

void addAssertionsFromFile(std::string assPath, Trace *trace, Context *c) {

  std::fstream ass(assPath);
  std::string line = "";
  std::vector<std::string> assStrs;
  while (std::getline(ass, line)) {
    assStrs.push_back(line);
  }

  messageInfo("Number of external assertions: " +
              std::to_string(assStrs.size()) + "\n");

  std::vector<Template *> templs;
  progresscpp::ParallelProgressBar pb;
  pb.addInstance(0, "Parsing assertions from file...", assStrs.size(), 70);

  for (size_t i = 0; i < assStrs.size(); i++) {
    Template *ass =
        hparser::parseTemplate(assStrs[i], trace, "Spot", DTLimits(), 0);
    // parallelize eval function with 2 threads
    ass->setL1Threads(2);
    // save
    templs.push_back(ass);

    pb.increment(0);
    pb.display();
  }
  pb.done(0);

  for (Template *t : templs) {
    //create an assertion by making a snapshot of a template
    Assertion *ass = new Assertion();
    t->fillContingency(ass->_ct, 0);
    ass->_toString =
        std::make_pair(t->getAssertion(), t->getColoredAssertion());
    auto loadedProps = t->getLoadedPropositions();
    ass->_complexity = getNumVariables(loadedProps);
    ass->_pRepetitions = getRepetitions(loadedProps);
    ass->fillValues(t);
    c->_assertions.push_back(ass);
  }
}

///used to parse the 'op' attribute in a numeric tag
std::unordered_set<ClsOp> parseClsOp(std::string op) {
  std::unordered_set<ClsOp> ret;
  op.erase(remove_if(op.begin(), op.end(), isspace), op.end());
  std::string word = "";
  for (size_t i = 0; i < op.size(); i++) {
    char c = op[i];
    messageErrorIf(c != ',' && c != 'r' && c != 'e' && c != 'g' && c != 'l' &&
                       c != ' ' && c != '<' && c != '>',
                   "Unrecognized token '" + std::string({c}) + "' in op");
    word += c;
    if (c == ',' || i == op.size() - 1) {
      if (c == ',')
        word.pop_back();

      if (word == "r") {
        ret.insert(ClsOp::Range);
      } else if (word == "ge") {
        ret.insert(ClsOp::GE);
      } else if (word == "le") {
        ret.insert(ClsOp::LE);
      } else if (word == "e") {
        ret.insert(ClsOp::E);
      } else {
        messageError("Unknown operator in '" + op +
                     "', available operators are 'r' (range), 'ge' (>=) 'le' "
                     "(<=), 'e' (==)");
      }
      word = "";
    }
  }
  return ret;
}
///used to parse the 'loc' attribute in a prop or numeric tag
std::vector<Location> parseLocation(std::string loc) {
  std::vector<Location> ret;
  loc.erase(remove_if(loc.begin(), loc.end(), isspace), loc.end());
  std::string word = "";
  for (size_t i = 0; i < loc.size(); i++) {
    char c = loc[i];
    messageErrorIf(c != ',' && c != 'a' && c != 'c' && c != 'd' && c != 't' &&
                       c != ' ',
                   "Unrecognized token '" + std::string({c}) + "' in loc");
    word += c;
    if (c == ',' || i == loc.size() - 1) {
      if (c == ',')
        word.pop_back();

      if (word == "a") {
        ret.push_back(Location::Ant);
      } else if (word == "c") {
        ret.push_back(Location::Con);
      } else if (word == "ac") {
        ret.push_back(Location::AntCon);
      } else if (word == "dt") {
        ret.push_back(Location::DecTree);
      } else {
        messageError(
            "Unknown option in '" + loc +
            "', available option are 'a' (antecedent), 'c' (consequent), 'ac' "
            "both antecedent and consequent, 'dt' (decision tree)");
      }
      word = "";
    }
  }
  return ret;
}
void ManualDefinition::mineContexts(Trace *trace,
                                    std::vector<Context *> &contexts) {

  std::vector<rapidxml::xml_node<> *> contextsTag;
  getNodesFromName(_configuration, "context", contextsTag);
  messageErrorIf(contextsTag.empty(), "No contexts defined");

  for (auto &contextTag : contextsTag) {
    auto contextName = getAttributeValue(contextTag, "name", "");
    auto language = getAttributeValue(contextTag, "language", "Spot");
    Context *context = new Context(contextName, language);

    // templates
    std::vector<rapidxml::xml_node<> *> templatesTag;
    getNodesFromName(contextTag, "template", templatesTag);
    for (auto &templateTag : templatesTag) {
      auto exp = getAttributeValue(templateTag, "exp", "");
      auto check = getAttributeValue(templateTag, "check", "0");
      auto dtLimitsStr =
          getAttributeValue(templateTag, "dtLimits", "3W,3D,3A,!O,N,-0.1E,R");
      //      messageWarningIf(getAttributeValue(templateTag, "dtLimits", "") == "", "dtLimits is undefined in template '" + exp +
      //                           "', using default "
      //                           "'3W,3D,3A,!O,N,0.1E,R'");
      DTLimits dtLimits = hparser::parseLimits(dtLimitsStr);
      //  debug
      // std::cout << "maxDepth:" << dtLimits._maxDepth << "\n";
      // std::cout << "maxWidth:" << dtLimits._maxWidth << "\n";
      // std::cout << "maxAll:" << dtLimits._maxAll << "\n";
      // std::cout << "dtRange:" << dtLimits._dtRange << "\n";
      // std::cout << "isRandomConstructed:" << dtLimits._isRandomConstructed
      //          << "\n";
      // std::cout << "saveOffset:" << dtLimits._saveOffset << "\n";
      // std::cout << "useNegatedProps:" << dtLimits._useNegatedProps << "\n";
      context->_templates.push_back(
          hparser::parseTemplate(exp, trace, language, dtLimits));
      messageErrorIf(check != "0" && check != "1",
                     "Unknown value for 'check' field");
      context->_templates.back()->_check = (check == "1" ? 1 : 0);
    }

    // get sort metrics
    std::vector<rapidxml::xml_node<> *> sortsTag;
    getNodesFromName(contextTag, "sort", sortsTag);
    for (auto &sortTag : sortsTag) {
      auto exp = getAttributeValue(sortTag, "exp", "0");
      auto name = getAttributeValue(sortTag, "name", "default");
      auto range = getAttributeValue(sortTag, "range", "[0,1]");

      context->_sort.push_back(hparser::parseMetric(name, exp, trace));
    }

    // get filter metrics
    std::vector<rapidxml::xml_node<> *> filtersTag;
    getNodesFromName(contextTag, "filter", filtersTag);
    for (auto &filterTag : filtersTag) {
      auto exp = getAttributeValue(filterTag, "exp", "0");
      auto name = getAttributeValue(filterTag, "name", "default");
      auto th = getAttributeValue(filterTag, "threshold", "0");
      context->_filter.emplace_back(hparser::parseMetric(name, exp, trace),
                                    std::stod(th));
    }

    // get propositions
    std::vector<rapidxml::xml_node<> *> propsTag;
    getNodesFromName(contextTag, "prop", propsTag);
    for (auto &propTag : propsTag) {
      auto exp = getAttributeValue(propTag, "exp", "");
      messageErrorIf(exp == "", "exp attribute is empty");
      auto locStr = getAttributeValue(propTag, "loc", "");
      if (locStr == "") {
        messageWarning("loc is empty in exp '" + exp +
                       "', using 'a, c, ac, dt'");
        locStr = "a, c, ac, dt";
      }

      auto locs = parseLocation(locStr);
      for (auto loc : locs) {
        context->_props.push_back(std::make_pair(
            new CachedProposition(hparser::parseProposition(exp, trace)), loc));
      }
    }

    // get numerics
    std::vector<rapidxml::xml_node<> *> numsTag;
    getNodesFromName(contextTag, "numeric", numsTag);

    if (!numsTag.empty()) {
      progresscpp::ParallelProgressBar pb;
      pb.addInstance(0, "Elaborating numerics (" + contextName + ")",
                     numsTag.size(), 70);
      for (auto &numTag : numsTag) {
        CachedAllNumeric *nn;
        auto exp = getAttributeValue(numTag, "exp", "");
        messageErrorIf(exp == "", "exp attribute is empty in numeric exp");
        auto locStr = getAttributeValue(numTag, "loc", "");
        if (locStr == "") {
          messageWarning("loc is empty in numeric exp '" + exp +
                         "', using 'a, c, ac, dt'\n\n");
          locStr = "a, c, ac, dt";
        }
        auto locs = parseLocation(locStr);
        double clsEffort =
            std::stod(getAttributeValue(numTag, "clsEffort", "0.3"));
        std::unordered_set<ClsOp> clsop =
            parseClsOp(getAttributeValue(numTag, "op", "r, e"));

        // convert to proposition to avoid having multiple parsers for similar
        // things
        Proposition *np = hparser::parseProposition(exp, trace);
        messageErrorIf(dynamic_cast<LogicToBool *>(np) == nullptr &&
                           dynamic_cast<NumericToBool *>(np) == nullptr,
                       "Exp " + exp + " is not of numeric or logic type");
        // retrieve the numeric expression
        if (dynamic_cast<LogicToBool *>(np) != nullptr) {
          nn = new expression::CachedAllNumeric(
              &dynamic_cast<LogicToBool *>(np)->getItem(), clsEffort, clsop);
          dynamic_cast<LogicToBool *>(np)->popItem();
        } else if (dynamic_cast<NumericToBool *>(np) != nullptr) {
          nn = new expression::CachedAllNumeric(
              &dynamic_cast<NumericToBool *>(np)->getItem(), clsEffort, clsop);
          dynamic_cast<NumericToBool *>(np)->popItem();
        } else {
          messageError("Bad type in CachedAllNumeric");
        }

        delete np;

        for (auto &loc : locs) {
          if (loc != Location::DecTree) {
            //generate props though clustering using the whole trace
            std::vector<size_t> ivs;
            for (size_t i = 0; i < trace->getLength(); i++) {
              ivs.push_back(i);
            }
            auto props = genPropsThroughClustering(ivs, nn, trace->getLength());
            for (auto p : props) {
              context->_props.emplace_back(p, loc);
            }
          } else {
            context->_numerics.push_back(nn);
          }
        }

        //delete nn if it is no longer used
        if (std::find(begin(locs), end(locs), Location::DecTree) == locs.end())
          delete nn;

        pb.increment(0);
        pb.display();
      }
      pb.done(0);
    }

    contexts.push_back(context);
  }

  if (clc::includeAss != "") {
    auto pc = std::find_if(begin(contexts), end(contexts),
                           [](Context *c) { return c->_name == "external"; });
    Context *external_context;
    if (pc == end(contexts)) {
      external_context = new Context("external");
      contexts.push_back(external_context);
    } else {
      messageInfo("Inserting external assertions into the 'external' context");
      external_context = *pc;
    }
    addAssertionsFromFile(clc::includeAss, trace, external_context);
  }
}

} // namespace harm
