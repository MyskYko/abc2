#include "global.h"

#ifdef PROFILE_MODE

#include "internal.hpp"

namespace CaDiCaL {

bool Internal::propagate_stable () {
  CADICAL_assert (stable);
  START (propstable);
  bool res = propagate ();
  STOP (propstable);
  return res;
}

void Internal::analyze_stable () {
  CADICAL_assert (stable);
  START (analyzestable);
  analyze ();
  STOP (analyzestable);
}

int Internal::decide_stable () {
  CADICAL_assert (stable);
  return decide ();
}

}; // namespace CaDiCaL

#else
int stable_if_not_profile_mode_dummy;
#endif
