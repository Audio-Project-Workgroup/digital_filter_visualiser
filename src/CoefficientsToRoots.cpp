// #pragma once
// #include <juce_audio_processors/juce_audio_processors.h>
// #include <juce_dsp/juce_dsp.h>

#include "CoefficientsToRoots.h"

#include <algorithm>
#include <cmath>

namespace CoefficientsToRoots
{

  static c128
  evaluatePolynomial(const Coefficients &coeffs, c128 pt)
  {
    c128 result = coeffs[0];
    for(size_t i = 1; i < coeffs.size(); ++i)
    {
      auto coeff = coeffs[i];
      result = pt*result + coeff;
    }

    return result;
  }

  static c128
  evaluateComplexPolynomial(const ComplexCoefficients &coeffs, c128 pt)
  {
    c128 result = coeffs[0];
    for(size_t i = 1; i < coeffs.size(); ++i)
    {
      auto coeff = coeffs[i];
      result = pt*result + coeff;
    }

    return result;
  }

  static c128
  dividePolynomial(const Coefficients &coeffs, c128 pt, ComplexCoefficients &quotient)
  {
    quotient.resize(coeffs.size());

    c128 result = coeffs[0];
    quotient[0] = result;

    for(size_t i = 1; i < coeffs.size(); ++i)
    {
      auto coeff = coeffs[i];
      result = pt*result + coeff;

      if(i < coeffs.size() - 1)
      { quotient[i] = result; }
    }

    return result;
  }

  static c128
  divideComplexPolynomial(const ComplexCoefficients &coeffs, c128 pt, ComplexCoefficients &quotient)
  {
    quotient.resize(coeffs.size());

    c128 result = coeffs[0];
    quotient[0] = result;

    for(size_t i = 1; i < coeffs.size(); ++i)
    {
      auto coeff = coeffs[i];
      result = pt*result + coeff;

      if(i < coeffs.size() - 1)
      { quotient[i] = result; }
    }

    return result;
  }

  static void
  dividePolynomialByRoot(const Coefficients &coeffs, Root pt, ComplexCoefficients &remainders)
  {
    jassert(pt.order <= int(coeffs.size()));
    jassert(pt.order == int(remainders.size()));

    auto remSize = static_cast<int>(remainders.size());

    ComplexCoefficients quotientBacking(coeffs.size() - 1);
    ComplexCoefficients *quotient = &quotientBacking;
    remainders[static_cast<size_t>(remSize - 1)] = dividePolynomial(coeffs, pt.value, *quotient);
    if(pt.order == 1) return;

    ComplexCoefficients ccoeffsBacking(coeffs.size() - 1);
    ComplexCoefficients *ccoeffs= &quotientBacking;
    quotient = &ccoeffsBacking;
    quotient->resize(quotient->size() - 1);

    for(int n = 1; n < pt.order; ++n)
    {
      remainders[static_cast<size_t>(remSize - 1 - n)] = divideComplexPolynomial(*ccoeffs, pt.value, *quotient);
      std::swap(ccoeffs, quotient);
      quotient->resize(quotient->size() - 1);
    }
  }

  static Root
  mergeRoot(Root oldRoot, Root addedRoot)
  {
    Root newRoot;
    if(juce::exactlyEqual(oldRoot.value.imag(), 0.0) == juce::exactlyEqual(addedRoot.value.imag(), 0.0))
    {
      newRoot.value = (oldRoot.value*c128(oldRoot.order, 0) + addedRoot.value) / c128(oldRoot.order + 1, 0);
      newRoot.order = oldRoot.order + 1;
    }
    else
    {
      c128 reVal = (!juce::exactlyEqual(oldRoot.value.imag(), 0.0)) ? addedRoot.value : oldRoot.value;
      c128 imVal = (!juce::exactlyEqual(oldRoot.value.imag(), 0.0)) ? c128(oldRoot.value.real(), 0) : c128(addedRoot.value.real(), 0);
      int reOrder = (!juce::exactlyEqual(oldRoot.value.imag(), 0.0)) ? addedRoot.order : oldRoot.order;
      int imOrder = (!juce::exactlyEqual(oldRoot.value.imag(), 0.0)) ? oldRoot.order : addedRoot.order;

      newRoot.value = (reVal*c128(reOrder, 0) + c128(2, 0)*imVal*c128(imOrder, 0)) / c128(reOrder + 2*imOrder, 0);
      newRoot.order = reOrder + 2*imOrder;
    }

    return newRoot;
  }

  static c128
  centerOfCircleThroughPoints(c128 z0, c128 z1, c128 z2)
  {
    // TODO: check for colinearity, duplicates

    c128 w = (z2 - z0) / (z1 - z0);
    c128 c = (z1 - z0)*((w - std::norm(w))/(w - std::conj(w))) + z0;
    return c;
  }

#include "CoefficientsToRoots.QR.cpp"
#include "CoefficientsToRoots.Aberth.cpp"

}
