#pragma once

class StateSerializer
{
public:
	static std::shared_ptr<juce::XmlElement> exportCoefficients(
		FilterState* state);

	static std::shared_ptr<juce::XmlElement> exportProcessorChainParameters(
		FullState<float>* state);
};

// NOTE(ry): I need to put this here so my editor doesn't screw with the style of this file
/* Local Variables: */
/* mode: c++ */
/* tab-width: 8 */
/* c-basic-offset: 8 */
/* indent-tabs-mode: t */
/* buffer-file-coding-system: undecided-unix */
/* End: */
