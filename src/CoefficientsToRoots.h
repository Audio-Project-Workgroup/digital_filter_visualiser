#pragma once

#include "FilterState.h"
#include <vector>
#include <utility>

#define DEBUG_C2R 1

namespace CoefficientsToRoots
{
  struct Root
  {
    Root(c128 v, int o) : value(v), order(o) {}

    c128 value;
    int order;
  };

  using Coefficients = std::vector<double>;
  using SolutionSet = std::vector<Root>;
  using SolverFn = SolutionSet (*)(Coefficients);

#define SOLVER_DEFINE(Name)\
  }\
  static CoefficientsToRoots::SolverFn constexpr Name##Solve = &CoefficientsToRoots::Name::Solve;\
  namespace CoefficientsToRoots\
  {\

#include "CoefficientsToRoots.QR.h"
#include "CoefficientsToRoots.Aberth.h"

}
