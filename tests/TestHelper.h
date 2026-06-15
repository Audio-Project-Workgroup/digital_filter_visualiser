#pragma once
#include "../src/FilterState.h"

struct TestRootSpecification
{
  int order;
  double valRe;
  double valIm;
};

class TestHelper
{
public:
    static void makeFilterState(
        FilterState* state,
        std::vector<TestRootSpecification>& roots,
        float gain)
    {
	    state->clear();
        state->gain.setValue(gain, nullptr);
        for (auto& r : roots)
            auto sr = state->add(r.order, { r.valRe, r.valIm });
    }
};
