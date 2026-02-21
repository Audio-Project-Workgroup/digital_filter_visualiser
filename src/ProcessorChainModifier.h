#pragma once
#include "FilterState.h"
#include "ProcessorChain.h"
#include "PluginProcessor.h"

class ProcessorChainModifier
{
public:
	static void RootsToJuceCoeffs(
		FilterState& state,
		FullState<float>* processorState,
		juce::dsp::ProcessSpec& spec)
	{
		struct PolesIndexesWithKeys
		{
			int index;
			double key;
		};

		const int channelSize = processorState->size();
		if (channelSize == 0)
			return;

		const int polesSize = state.poles.size();
		const auto zerosSize = state.zeros.size();

		int zerosBiquadSize = 0;
		for (auto* item : state.zeros)
		{
			jassert(std::abs(item->value.get()) != 0);
			zerosBiquadSize += item->order.get();
		}

		// 1. Sort poles
		// (zero magnitude poles will be in the end of the vector)
		std::vector<PolesIndexesWithKeys> polesIndexesWithKeys(polesSize);
		for (int i = 0; i < polesSize; i++)
			polesIndexesWithKeys[i] = { i, EvaluatePole(state.poles[i]) };
		std::sort(polesIndexesWithKeys.begin(), polesIndexesWithKeys.end(),
			[](const PolesIndexesWithKeys a, const PolesIndexesWithKeys b)
			{
				return a.key > b.key;
			});

		// 2. Find the best zero pair for each pole.
		std::vector<int> usedZeros(zerosSize, false);
		std::vector<int> zerosIndexes;
		zerosIndexes.reserve(zerosBiquadSize);

		int delayCount = 0;
		int firstNullPoleIndex = 0;
		int bestIndex = -1;
		int polesBiquadSize = 0;
		for (int i = 0; i < polesSize; i++)
		{
			auto* pole = state.poles[polesIndexesWithKeys[i].index];
			const int poleOrder = std::abs(pole->order.get());

			if (std::abs(pole->value.get()) == 0)
			{
				delayCount += poleOrder;
				continue;
			}

			firstNullPoleIndex++;
			polesBiquadSize += poleOrder;
			for (int j = 0; j < poleOrder; j++)
			{
				bestIndex = FindBestZeroIndexPairForPole(pole, state.zeros, usedZeros);
				if (bestIndex == -1)
					break;
				usedZeros[bestIndex]++;
				zerosIndexes.push_back(bestIndex);
			}
			if (bestIndex == -1)
				break;
		}

		// 3. Put other zeros to the end of a cascade.
		if (polesBiquadSize == 0 || bestIndex != -1)
		{
			for (auto i = 0; i < zerosSize; i++)
			{
				const int order = state.zeros[i]->order.get();
				for (int j = usedZeros[i]; j < order; j++)
					zerosIndexes.push_back(i);
				usedZeros[i] = order;
			}
		}

		// 4. IIR coefficients cascade
		std::vector<juce::dsp::IIR::Coefficients<float>*> iirCoeffs(polesBiquadSize);
		float b0, b1, b2, a0, a1, a2;
		int zerosIndexesIndex = 0;
		for (int i = 0; i < firstNullPoleIndex; i++)
		{
			auto* pole = state.poles[polesIndexesWithKeys[i].index];
			const int poleOrder = std::abs(pole->order.get());
			calculateCoefficients(pole, a0, a1, a2);
			for (int j = 0; j < poleOrder; j++)
			{
				if (zerosIndexesIndex == zerosBiquadSize)
				{
					b0 = 1.0;
					b1 = b2 = 0;
				}
				else
				{
					auto* zero = state.zeros[zerosIndexes[zerosIndexesIndex++]];
					calculateCoefficients(zero, b0, b1, b2);
				}
				if (a0 == 0 && b0 == 0) // 1-order filter
					iirCoeffs[i] = new juce::dsp::IIR::Coefficients<float>{ b1, b2, a1, a2 };
				else // 2-order filter
					iirCoeffs[i] = new juce::dsp::IIR::Coefficients<float>{ b0, b1, b2, a0, a1, a2 };
			}
		}
		std::reverse(iirCoeffs.begin(), iirCoeffs.end()); // TODO: check and optimize

		// 5. Create FIR coefficients cascade
		const int firCoeffSize = std::max(0, zerosBiquadSize - zerosIndexesIndex);
		std::vector<juce::dsp::FIR::Coefficients<float>*> firCoeffs(firCoeffSize);
		std::array<float, 3> bCoeff{ 0 };
		for (int i = 0; i < firCoeffSize; i++)
		{
			auto* zero = state.zeros[zerosIndexes[zerosIndexesIndex++]];
			calculateCoefficients(zero, bCoeff[0], bCoeff[1], bCoeff[2]);
			firCoeffs[i] = new juce::dsp::FIR::Coefficients<float>{ bCoeff.data(), 3 };
		}

		// 6. Set processors parameters
		for (int ch = 0; ch < channelSize; ch++)
		{
			auto* proc = (*processorState)[ch];

			proc->delay.setDelay(delayCount);
			proc->delay.reset();
			proc->delay.prepare(spec);

			auto& iirCascade = proc->iirCascade;
			for (int i = 0; i < polesBiquadSize; i++)
			{
				//auto& coeffsArr = iirCoeffs[i];
				if (i == iirCascade.size())
					iirCascade.add(new juce::dsp::IIR::Filter<float>{ iirCoeffs[i] });
				else
				{
					iirCascade[i]->coefficients = iirCoeffs[i];
					iirCascade[i]->reset();
				}
				iirCascade[i]->prepare(spec);
			}
			iirCascade.removeRange(polesBiquadSize, iirCascade.size() - polesBiquadSize); // TODO: don't delete objects for optimization?

			auto& firCascade = proc->firCascade;
			for (int i = 0; i < firCoeffSize; i++)
			{
				if (i == firCascade.size())
					firCascade.add(new juce::dsp::FIR::Filter<float>{ firCoeffs[i] });
				else
				{
					firCascade[i]->coefficients = firCoeffs[i];
					firCascade[i]->reset();
				}
				firCascade[i]->prepare(spec);
			}
			firCascade.removeRange(firCoeffSize, firCascade.size() - firCoeffSize); // TODO: don't delete objects for optimization?
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
			RootsToJuceCoeffs(processor.state, processor.activeState, processor.spec);
		}
		// If the updated state is already loaded 
		// but the sound was not fully processed using this state
		// then another update should be cancelled.
		else if (!processor.isNewStateReady.load())
		{
			processor.isPendingStateUsed.store(true);
			RootsToJuceCoeffs(processor.state, processor.pendingState, processor.spec);
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
		const double mag = std::abs(pole->value.get());
		const double angleAbs = std::abs(std::arg(pole->value.get()));
		const double q = mag == 0 ? 0.5 : -0.5 * angleAbs / std::log(mag);
		return q * qCoeff + mag * magCoeff + angleAbs * angleCoeff;
	}

	static int FindBestZeroIndexPairForPole(
		const FilterRoot* pole,
		juce::OwnedArray<FilterRoot>& zeros,
		std::vector<int>& usedZeros)
	{
		const auto size = usedZeros.size();
		const double poleAngle = std::arg(pole->value.get());

		auto currentMinIndex = -1;
		auto currentUnitCircleMinIndex = -1;
		double currentMin = 100.0; // greater than maximum possible delta
		double currentUnitCircleMin = 100.0; // greater than maximum possible delta
		for (auto i = 0; i < size; i++)
		{
			if (usedZeros[i] == zeros[i]->order.get())
				continue;

			const double currentDelta = angleDiffAbs(poleAngle, std::arg(zeros[i]->value.get()));
			const double currentMag = std::abs(zeros[i]->value.get());

			if (currentDelta < currentMin)
			{
				currentMinIndex = i;
				currentMin = currentDelta;
			}

			if (std::abs(currentMag - 1) <= magnitudeThreshold &&
				currentDelta < currentUnitCircleMin)
			{
				currentUnitCircleMinIndex = i;
				currentUnitCircleMin = currentDelta;
			}
		}

		if (currentUnitCircleMinIndex != -1 &&
			angleDiffAbs(currentUnitCircleMin, poleAngle) <= angleSimilarityThreshold)
			return currentUnitCircleMinIndex;

		return currentMinIndex;
	}

	static inline void calculateCoefficients(
		FilterRoot* root,
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
		if (im == 0)
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