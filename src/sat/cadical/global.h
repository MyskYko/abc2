#ifndef ABC_SAT_CADICAL_GLOBAL_HPP_
#define ABC_SAT_CADICAL_GLOBAL_HPP_

#define CADICAL_NDEBUG

#define NBUILD
#define QUIET
#define NCONTRACTS
#define NTRACING
#define NCLOSEFROM

#ifdef CADICAL_NDEBUG
#define CADICAL_assert(ignore) ((void)0)
#else
#define CADICAL_assert(cond) assert(cond)
#endif

#include "misc/util/abc_global.h"

#endif
