#pragma once
#include "FilterState.h"
#include "ProcessorChain.h"
#include "RootsToCoefficients.h"

class StateSerializer
{
public:
	static std::shared_ptr<juce::XmlElement> exportCoefficients(
		FilterState* state)
	{
		auto zeroCoeffs = RootsToCoefficients::CalculatePolynomialCoefficientsFrom(state->zeros);
		auto poleCoeffs = RootsToCoefficients::CalculatePolynomialCoefficientsFrom(state->poles);
		auto root = std::make_shared<juce::XmlElement>("filter_state");

		auto* subElement = root->createNewChildElement("numerator_coefficients");
		juce::StringArray bCoeffStrArray;
		for (int i = 0; i < zeroCoeffs.size(); i++)
			bCoeffStrArray.add(juce::String(zeroCoeffs[static_cast<size_t>(i)]));
		subElement->addTextElement(bCoeffStrArray.joinIntoString(", "));

		subElement = root->createNewChildElement("denominator_coefficients");
		juce::StringArray aCoeffStrArray;
		for (int i = 0; i < poleCoeffs.size(); i++)
			aCoeffStrArray.add(juce::String(poleCoeffs[static_cast<size_t>(i)]));
		subElement->addTextElement(aCoeffStrArray.joinIntoString(", "));

		return root;
	}

	static std::shared_ptr<juce::XmlElement> exportProcessorChainParameters(
		FullState<float>* state)
	{
		auto* chain = state->getFirst();
		auto root = std::make_shared<juce::XmlElement>("processor_chain_parameters");

		auto* subElement = root->createNewChildElement("delay_samples");
		subElement->addTextElement(juce::String(chain->delay.getDelay()));

		auto* iirCascadeElement = root->createNewChildElement("iir_cascade");
		for (auto* iirFilter : chain->iirCascade)
		{
			auto* filterElement = iirCascadeElement->createNewChildElement("iir_filter");
			auto& iirCoeffs = iirFilter->coefficients->coefficients;

			auto* bCoeffsElement = filterElement->createNewChildElement("numerator_coefficients");
			juce::StringArray bCoeffStrArray;
			for (int i = 0; i <= iirCoeffs.size() / 2; i++)
				bCoeffStrArray.add(juce::String(iirCoeffs[i]));
			bCoeffsElement->addTextElement(bCoeffStrArray.joinIntoString(", "));

			auto* aCoeffsElement = filterElement->createNewChildElement("denominator_coefficients");
			juce::StringArray aCoeffStrArray{ "1" };
			for (int i = iirCoeffs.size() / 2 + 1; i < iirCoeffs.size(); i++)
				aCoeffStrArray.add(juce::String(iirCoeffs[i]));
			aCoeffsElement->addTextElement(aCoeffStrArray.joinIntoString(", "));
		}

		subElement = root->createNewChildElement("fir_filter");
		subElement = subElement->createNewChildElement("coefficients");
		juce::StringArray firCoeffStrArray;
		auto& firCoeffs = chain->firFilter->coefficients->coefficients;
		for (auto val: firCoeffs)
			firCoeffStrArray.add(juce::String(val));
		subElement->addTextElement(firCoeffStrArray.joinIntoString(", "));

		subElement = root->createNewChildElement("gain");
		subElement->addTextElement(juce::String(chain->gain.getGainLinear()));

		return root;
	}
};

// NOTE(ry): I need to put this here so my editor doesn't screw with the style of this file
/* Local Variables: */
/* mode: c++ */
/* tab-width: 8 */
/* c-basic-offset: 8 */
/* indent-tabs-mode: t */
/* buffer-file-coding-system: undecided-unix */
/* End: */
