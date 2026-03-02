#pragma once
#include "FilterState.h"
#include "ProcessorChain.h"
#include "PluginProcessor.h"

class ProcessorChainModifier
{
public:
	static void RootsToJuceCoeffs(
		FilterState* state,
		FullState<float>* processorState,
		juce::dsp::ProcessSpec& spec)
	{
		struct PolesIndexesWithKeys
		{
			std::size_t index;
			double key;
		};

		const int channelSize = processorState->size();
		if (channelSize == 0)
			return;

		const auto polesSize = static_cast<std::size_t>(state->poles.size());
		const auto zerosSize = static_cast<std::size_t>(state->zeros.size());

		int zerosBiquadSize = 0;
		for (auto* item : state->zeros)
		{
			// TODO(ry): I don't think this assert should be
			// here. It's possible to have a zero root at z=0 when
			// there are poles elsewhere
			jassert(!item->isAtZero());
			zerosBiquadSize += item->order.get();
		}

		// 1. Sort non-null poles by priority for pairing with zeros
		// (they should be cascaded in reverse order);
		// calculate delay count.
		int delayCount = 0;
		std::vector<PolesIndexesWithKeys> polesIndexesWithKeys;
		polesIndexesWithKeys.reserve(polesSize);
		for (std::size_t i = 0; i < polesSize; i++)
		{
			auto* pole = state->poles[i];
			if(pole->isAtZero())
				delayCount += std::abs(pole->order.get());
			else
				polesIndexesWithKeys.push_back({ i, EvaluatePole(pole) });
		}
		std::sort(polesIndexesWithKeys.begin(), polesIndexesWithKeys.end(),
			[](const PolesIndexesWithKeys a, const PolesIndexesWithKeys b)
			{
				return a.key > b.key;
			});
		const auto nonNullPolesSize = polesIndexesWithKeys.size();

		// 2. Find the best zero pair for each pole
		// and calculate IIR coefficients
		std::vector<int> usedZeros(zerosSize, false);
		std::vector<juce::dsp::IIR::Coefficients<float>*> iirCoeffs;

		int bestZeroIndex = -1;
		for (std::size_t i = 0; i < nonNullPolesSize; i++)
		{
			auto* pole = state->poles[polesIndexesWithKeys[i].index];
			const int poleOrder = std::abs(pole->order.get());

			for (int j = 0; j < poleOrder; j++)
			{
				bool shouldPoleBePairedWithDelay, shouldEqualPoleBeTaken;

				FindBestZeroIndexPairForPole(
					pole,
					state->zeros,
					usedZeros,
					poleOrder - j > 1,
					delayCount != 0,
					bestZeroIndex,
					shouldPoleBePairedWithDelay,
					shouldEqualPoleBeTaken);

				iirCoeffs.push_back(CalculateIirCoefficients(
					pole,
					state->zeros,
					bestZeroIndex,
					shouldPoleBePairedWithDelay,
					shouldEqualPoleBeTaken));

				if (shouldPoleBePairedWithDelay)
					delayCount--;
				if (shouldEqualPoleBeTaken)
					j++;
				if (bestZeroIndex != -1)
					usedZeros[static_cast<std::size_t>(bestZeroIndex)]++;
			}
		}
		const auto iirFiltersSize = iirCoeffs.size();

		// 3. Calculate FIR coefficients for each non-paired zero.
		std::vector<juce::dsp::FIR::Coefficients<float>*> firCoeffs;
		std::array<float, 3> bCoeff{ 0 };
		for (std::size_t i = 0; i < zerosSize; i++)
		{
			auto* zero = state->zeros[i];
			const int order = zero->order.get();
			const bool isReal = zero->isReal();
			for (int j = usedZeros[i]; j < order; j++)
			{
				const bool shouldBeTakenTwice = isReal && order - j > 1;
				calculatePolynomialCoefficients(
					zero,
					shouldBeTakenTwice,
					bCoeff[0], bCoeff[1], bCoeff[2]);
				firCoeffs.push_back(new juce::dsp::FIR::Coefficients<float>{ bCoeff.data(), 3 });
				if (shouldBeTakenTwice)
					j++;
			}
		}
		const auto firFiltersSize = firCoeffs.size();

		// 4. Set processors parameters
		for (int ch = 0; ch < channelSize; ch++)
		{
			auto* proc = (*processorState)[ch];

			// 4a. Delay
			auto& delay = proc->delay;
			if (delay.getMaximumDelayInSamples() < delayCount)
				delay.setMaximumDelayInSamples(delayCount);
			delay.setDelay(delayCount);
			delay.reset();
			delay.prepare(spec);

			// 4b. IIR cascade
			auto& iirCascade = proc->iirCascade;
			auto iirCascadeSize = static_cast<std::size_t>(iirCascade.size());
			for (std::size_t i = 0 ; i < iirFiltersSize; i++)
			{
				const std::size_t iirCoeffsIndex = iirFiltersSize - 1 - i; // reverse order
				if (i == iirCascadeSize)
				{
					iirCascade.add(new juce::dsp::IIR::Filter<float>{ iirCoeffs[iirCoeffsIndex] });
					iirCascadeSize = static_cast<std::size_t>(iirCascade.size());
				}
				else
				{
					iirCascade[i]->coefficients = iirCoeffs[iirCoeffsIndex];
					iirCascade[i]->reset();
				}
				iirCascade[i]->prepare(spec);
			}
			iirCascade.removeRange(iirFiltersSize, iirCascadeSize - iirFiltersSize); // TODO: don't delete objects for optimization?

			// 4c. FIR cascade
			auto& firCascade = proc->firCascade;
			auto firCascadeSize = static_cast<std::size_t>(firCascade.size());
			for (std::size_t i = 0; i < firFiltersSize; i++)
			{
				if (i == firCascadeSize)
				{
					firCascade.add(new juce::dsp::FIR::Filter<float>{ firCoeffs[i] });
					firCascadeSize = static_cast<std::size_t>(firCascade.size());
				}
				else
				{
					firCascade[i]->coefficients = firCoeffs[i];
					firCascade[i]->reset();
				}
				firCascade[i]->prepare(spec);
			}
			firCascade.removeRange(firFiltersSize, firCascadeSize - firFiltersSize); // TODO: don't delete objects for optimization?

			//4d. Gain
			proc->gain.setGainLinear(state->gain.get());
		}
	}

	static void Process(AudioPluginAudioProcessor& processor)
	{
		// This prevents simultaneous Process and prepareToPlay working.
		std::unique_lock<std::mutex> lock(processor.stateMutex);

		// If playing is not started yet
		// then the changed state goes directly to the activeState.
		// This is to ensure that pendingState will never be used in processBlock
		// without spec preparation,
		// even if a user changes processing parameters before pushing Play button.
		if (!processor.isPrepared)
		{
			RootsToJuceCoeffs(processor.filterState.get(), processor.activeState, processor.spec);
		}
		// If the updated state is already loaded
		// but the sound was not fully processed using this state
		// then another update should be cancelled.
		else if (!processor.isNewStateReady.load())
		{
			processor.isPendingStateUsed.store(true);
			RootsToJuceCoeffs(processor.filterState.get(), processor.pendingState, processor.spec);
			processor.isPendingStateUsed.store(false);
			processor.isNewStateReady.store(true);
		}

		//// Calculate in double for displaying
		//auto z = Roots2CoeffsVector(processor.state.zeros);
		//auto p = Roots2CoeffsVector(processor.state.poles);
	}

private:
	static inline double EvaluatePole(const FilterRoot* pole)
	{
		const auto value = pole->value.get();
		const double mag = std::abs(value);
		const double angleAbs = std::abs(std::arg(value));
		const double q = juce::exactlyEqual(mag, 0.0) ? 0.5 : -0.5 * angleAbs / std::log(mag);
		return q * qCoeff + mag * magCoeff + angleAbs * angleCoeff;
	}

	static void FindBestZeroIndexPairForPole(
		const FilterRoot* pole,
		juce::OwnedArray<FilterRoot>& zeros,
		std::vector<int>& usedZeros,
		bool doesEqualPoleExist,
		bool doesDelayExist,
		int& bestZeroIndex,
		bool& shouldPoleBePairedWithDelay,
		bool& shouldEqualPoleBeTaken)
	{
		const auto usedZerosSize = usedZeros.size();
		const double poleAngle = std::arg(pole->value.get());
		const bool isPoleReal = pole->isReal();
		const bool shouldDenominatorBeFirstOrder =
			isPoleReal && !doesDelayExist && !doesEqualPoleExist;

		auto currentBestIndex = -1;
		auto currentUnitCircleBestIndex = -1;
		double currentMin = 100.0; // greater than maximum possible delta
		double currentUnitCircleMin = 100.0; // greater than maximum possible delta
		for (std::size_t i = 0; i < usedZerosSize; i++)
		{
			auto* zero = zeros[i];

			// Zero is already used with its order
			if (usedZeros[i] == zero->order.get())
				continue;

			// Check that numerator's order <= denominator's order,
			// otherwise don't get this zero.
			const bool isZeroReal = zero->isReal();
			if (shouldDenominatorBeFirstOrder && !isZeroReal)
				continue;

			const double currentDelta = angleDiffAbs(poleAngle, std::arg(zero->value.get()));
			const double currentMag = std::abs(zero->value.get());

			if (currentDelta < currentMin)
			{
				currentBestIndex = i;
				currentMin = currentDelta;
			}

			if (std::abs(currentMag - 1) <= magnitudeThreshold &&
				currentDelta < currentUnitCircleMin)
			{
				currentUnitCircleBestIndex = i;
				currentUnitCircleMin = currentDelta;
			}
		}

		if (currentUnitCircleBestIndex != -1 &&
			angleDiffAbs(currentUnitCircleMin, poleAngle) <= angleSimilarityThreshold)
			bestZeroIndex = currentUnitCircleBestIndex;
		else
			bestZeroIndex = currentBestIndex;

		// No zero to pair
		if (bestZeroIndex == -1)
		{
			shouldEqualPoleBeTaken = isPoleReal && doesEqualPoleExist;
			shouldPoleBePairedWithDelay = !shouldEqualPoleBeTaken && isPoleReal && doesDelayExist;
			return;
		}

		//const bool isBestZeroReal = zeros[bestZeroIndex]->value.im.get() == 0;
		const bool isBestZeroReal = zeros[bestZeroIndex]->isReal();

		// Pair 2-order pole with any zero
		// or 1-order pole with 1-order zero
		if (!isPoleReal || isBestZeroReal)
			shouldEqualPoleBeTaken = shouldPoleBePairedWithDelay = false;
		// Pair 2 equal 1-order poles with 2-order zero
		else if (doesEqualPoleExist)
		{
			shouldEqualPoleBeTaken = true;
			shouldPoleBePairedWithDelay = false;
		}
		// Pair 1-order pole and delay with 2-order zero
		else
		{
			jassert(doesDelayExist);
			shouldEqualPoleBeTaken = false;
			shouldPoleBePairedWithDelay = true;
		}
	}

	static juce::dsp::IIR::Coefficients<float>* CalculateIirCoefficients(
		FilterRoot* pole,
		juce::OwnedArray<FilterRoot>& zeros,
		int zeroIndex,
		bool shouldPoleBePairedWithDelay,
		bool shouldEqualPoleBeTaken)
	{
		const bool isPoleReal = pole->isReal();
		const bool shouldPoleBePaired = shouldPoleBePairedWithDelay || shouldEqualPoleBeTaken;
		jassert(isPoleReal || !shouldPoleBePaired);
		float b0, b1, b2, a0, a1, a2;

		if (shouldPoleBePairedWithDelay) // 1-order pole + delay (pole in null)
		{
			a0 = 1.f;
			a1 = -pole->value.re.get();
			a2 = 0.f;
		}
		else
			calculatePolynomialCoefficients(pole, shouldEqualPoleBeTaken, a0, a1, a2);

		if (zeroIndex != -1)
			calculatePolynomialCoefficients(zeros[zeroIndex], false, b0, b1, b2);
		else if (juce::exactlyEqual(a0, 0.f)) // no zeros, 1-order pole
		{
			b1 = 1.0;
			b0 = b2 = 0;
		}
		else // no zeros, 2-order pole
		{
			b0 = 1.0;
			b1 = b2 = 0;
		}

		if (juce::exactlyEqual(a0, 0.f)) // 1-order filter
		{
			jassert(juce::exactlyEqual(b0, 0.f) && !juce::exactlyEqual(a1, 0.f));
			return new juce::dsp::IIR::Coefficients<float>{ b1, b2, a1, a2 };
		}
		else // 2-order filter
		{
			return new juce::dsp::IIR::Coefficients<float>{ b0, b1, b2, a0, a1, a2 };
		}
	}

	static inline void calculatePolynomialCoefficients(
		FilterRoot* root,
		bool shouldBeTakenTwice,
		float& c0, float& c1, float& c2)
	{
		if (root == nullptr)
		{
			c2 = 1.0;
			c0 = c1 = 0;
			return;
		}
		const double re = root->value.re.get();
		const double im = root->value.im.get();

		const bool rootIsReal = juce::exactlyEqual(im, 0.0);
		jassert(rootIsReal || !shouldBeTakenTwice);

		if (rootIsReal && !shouldBeTakenTwice)
		{
			c2 = -re;
			c1 = 1.0;
			c0 = 0;
			return;
		}

		c2 = re * re + im * im;
		c1 = -2.0 * re;
		c0 = 1.0;
	}

	static inline double angleDiffAbs(double a, double b)
	{
		double d = a - b;
		d = std::fmod(d + pi, 2.0 * pi);
		if (d < 0)
			d += 2.0 * pi;
		return std::abs(d - pi);
	}

	static constexpr double pi = 3.14159265358979323846;
	static constexpr double angleSimilarityThreshold = 0.1 * pi;
	static constexpr double magnitudeThreshold = 1e-5;
	static constexpr double qCoeff = 0.6;
	static constexpr double magCoeff = 0.3;
	static constexpr double angleCoeff = 0.1;
};

// NOTE(ry): I need to put this here so my editor doesn't screw with the style of this file
/* Local Variables: */
/* mode: c++ */
/* tab-width: 8 */
/* c-basic-offset: 8 */
/* indent-tabs-mode: t */
/* buffer-file-coding-system: undecided-unix */
/* End: */
