#include "classes/atom/Atom.hh"
#include "visitors/ExpVisitor.hh"

namespace expression {

template <>
void BitSelector<LogicExpression, LogicExpression>::acceptVisitor(
    ExpVisitor &vis) {
  vis.visit(*this);
}

template <>
ULogic BitSelector<LogicExpression, LogicExpression>::evaluate(size_t time) {
  return (static_cast<ULogic>(((1 << (_upper_bound - _lower_bound + 1)) - 1)
                                << _lower_bound) &
          _e->evaluate(time));
}

template <>
bool BitSelector<LogicExpression, Proposition>::evaluate(size_t time) {
  //  messageErrorIf(_upper_bound != _lower_bound, "Single bit selector index
  //  error");

  return static_cast<ULogic>(((1 << (_upper_bound - _lower_bound + 1)) - 1)
                               << _lower_bound) &
         _e->evaluate(time);
}

} // namespace expression
