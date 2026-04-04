#pragma once

#include <cmath>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "PluginProcessor.h"

class PhaseFrequencyResponseViewer final :
    public juce::Component,
    juce::ChangeListener
{
public:
    PhaseFrequencyResponseViewer(AudioPluginAudioProcessor* _processor) :
		ampDb(minAmpDb),
		sampleRate(_processor->getSampleRate()),
        processor(_processor),
        zoomInButton("+"),
        zoomOutButton("-"),
        logScaleButton("Hz log scale")
    {
        processor->addChangeListener(this);
        processor->filterState->um->addChangeListener(this);

        zoomInButton.onClick = [this]
            {
                if (ampDb > minAmpDb)
                {
                    ampDb /= 2.f;
                    repaint();
                }
            };
        zoomOutButton.onClick = [this]
            {
                if (ampDb < maxAmpDb)
                {
                    ampDb *= 2.f;
                    repaint();
                }
            };
        logScaleButton.onClick = [this]
            {
                repaint();
            };
        logScaleButton.setClickingTogglesState(true);
        logScaleButton.setEnabled(sampleRate > 0);

        addAndMakeVisible(zoomInButton);
        addAndMakeVisible(zoomOutButton);
        addAndMakeVisible(logScaleButton);
    }

    ~PhaseFrequencyResponseViewer()
    {
        processor->filterState->um->removeChangeListener(this);
        processor->removeChangeListener(this);
    }

    void changeListenerCallback(juce::ChangeBroadcaster* source) override
    {
        if (source == processor->filterState->um)
            repaint();
        else if (source == processor)
        {
            sampleRate = processor->getSampleRate();
            logScaleButton.setEnabled(sampleRate > 0);
            repaint();
        }
    }

    void resized() override
    {
        zoomOutButton.setBounds(padding, padding, zoomButtonsSize, zoomButtonsSize);
        zoomInButton.setBounds(2 * padding + zoomButtonsSize, padding, zoomButtonsSize, zoomButtonsSize);
        logScaleButton.setBounds(getWidth() - plotPaddingRight - logScaleButtonWidth, padding, logScaleButtonWidth, zoomButtonsSize);
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        const auto bounds = getBounds();
        const auto width = bounds.getWidth() - plotPaddingLeft - plotPaddingRight;
        const auto height = bounds.getHeight() - plotPaddingTop - plotPaddingBottom;
        const float xLeft = static_cast<float>(plotPaddingLeft);
        const float xRight = static_cast<float>(xLeft + width);
        const float yTop = static_cast<float>(plotPaddingTop);
        const float yBottom = static_cast<float>(yTop + height);

        if (width < 2 || height < 2)
            return;

        juce::Rectangle<int> rect(plotPaddingLeft, plotPaddingTop, width, height);
        g.fillAll(backgroundColor);
        g.setColour(gridColour);
        g.drawRect(rect, 1);

        // Angle values for each plot x point
        std::vector<double> angles(static_cast<std::size_t>(width));
        const bool isLogScale = logScaleButton.getToggleState();
        if (isLogScale)
        {
            const float maxAngle = juce::MathConstants<float>::pi;
            const float minAngle = maxAngle * minFreq / (sampleRate / 2);
            const float logMaxAngle = std::log10(maxAngle);
            const float logMinAngle = std::log10(minAngle);
            const float angleCoeff = (logMaxAngle - logMinAngle) / width;
            for (int i = 0; i < width; i++)
                angles[static_cast<size_t>(i)] = std::pow(10.f, angleCoeff * i + logMinAngle);
        }
        else
        {
            const double angleStep = juce::MathConstants<double>::pi / width;
            for (int i = 0; i < width; i++)
                angles[static_cast<size_t>(i)] = angleStep * i;
        }

        // amplitudes & phases
        std::vector<double> amplitudes, phases;
        calculate(angles, amplitudes, phases);
        for (int i = 0; i < width; i++)
            amplitudes[static_cast<size_t>(i)] = juce::Decibels::gainToDecibels(amplitudes[static_cast<size_t>(i)]);

        // Y grid
        g.setColour(lineColour);
        g.drawText(
            juce::String(static_cast<int>(ampDb)) + " dB",
            padding,
            plotPaddingTop - textHeight / 2,
            plotPaddingLeft - 3 * padding,
            textHeight,
            juce::Justification::centredRight);
        g.drawText(
            "0",
            padding,
            plotPaddingTop + (height - textHeight) / 2,
            plotPaddingLeft - 3 * padding,
            textHeight,
            juce::Justification::centredRight);
        g.drawText(
            juce::String(static_cast<int>(-ampDb)) + " dB",
            padding,
            plotPaddingTop + height - textHeight / 2,
            plotPaddingLeft - 3 * padding,
            textHeight,
            juce::Justification::centredRight);

        // X grid
        const int hzTextY = static_cast<int>(yBottom) + padding;
        if (isLogScale)
        {
            g.setColour(lineColour);
            g.drawText(
                juce::String(static_cast<int>(minFreq)),
                plotPaddingLeft - textWidth / 2,
                hzTextY,
                textWidth,
                textHeight,
                juce::Justification::centred);
            g.drawText(
                juce::String(static_cast<int>(sampleRate / 2)),
                static_cast<int>(xRight) - textWidth / 2,
                hzTextY,
                textWidth,
                textHeight,
                juce::Justification::centred);
            g.drawText(
                " Hz",
                static_cast<int>(xRight) + textWidth / 2,
                hzTextY,
                textWidth,
                textHeight,
                juce::Justification::centredLeft);

            const float maxFreq = sampleRate / 2;
            const float logMaxFreq = std::log10(maxFreq);
            const float logMinFreq = std::log10(minFreq);
            const float freqCoeff = width / (logMaxFreq - logMinFreq);

            int freq = static_cast<int>(minFreq * 10);
            while (freq < maxFreq)
            {
                int x = freqCoeff * (std::log10(freq) - logMinFreq) + plotPaddingLeft;
                g.setColour(gridColour);
                g.drawLine(x, yTop, x, yBottom);
                if (xRight - x > textWidth)
                {
                    g.setColour(lineColour);
                    g.drawText(
                        juce::String(freq),
                        x - textWidth / 2,
                        hzTextY,
                        textWidth,
                        textHeight,
                        juce::Justification::centred);
                }
                freq *= 10;
            }
        }
        else
        {
            g.setColour(lineColour);
            g.drawText(
                "0",
                plotPaddingLeft - textWidth / 2,
                hzTextY,
                textWidth,
                textHeight,
                juce::Justification::centred);
            g.drawText(
                juce::exactlyEqual(sampleRate, 0.0) ? "pi/2" : juce::String(static_cast<int>(sampleRate / 4)),
                plotPaddingLeft + (width - textWidth) / 2,
                hzTextY,
                textWidth,
                textHeight,
                juce::Justification::centred);
            g.drawText(
                juce::exactlyEqual(sampleRate, 0.0) ? "pi" : juce::String(static_cast<int>(sampleRate / 2)),
                static_cast<int>(xRight) - textWidth / 2,
                hzTextY,
                textWidth,
                textHeight,
                juce::Justification::centred);
            if (!juce::exactlyEqual(sampleRate, 0.0))
                g.drawText(
                    " Hz",
                    static_cast<int>(xRight) + textWidth / 2,
                    hzTextY,
                    textWidth,
                    textHeight,
                    juce::Justification::centredLeft);

            g.setColour(gridColour);
            int x = plotPaddingLeft + width / 2;
            g.drawLine(x, yTop, x, yBottom);
        }

        // drawing values
        {
            // clipping inside curly brackets
            const juce::Graphics::ScopedSaveState state(g);
            g.reduceClipRegion(rect);

            juce::ColourGradient gradient(
                juce::Colours::red.withAlpha(0.75f), 0.0f, rect.getY(),
                juce::Colours::green.withAlpha(0.75f), 0.0f, rect.getBottom(),
                false);
            gradient.addColour(0.5, juce::Colours::yellow.withAlpha(0.75f));
            g.setGradientFill(gradient);

            juce::Path path;
            auto x = xLeft;
            auto y = juce::jmap(static_cast<float>(amplitudes[0]), -ampDb, ampDb, yBottom, yTop);
            g.drawLine(x, rect.getCentreY(), x, y);
            path.startNewSubPath(x, y);
            for (int i = 1; i < width; i++)
            {
                x++;
                y = juce::jmap(static_cast<float>(amplitudes[static_cast<size_t>(i)]), -ampDb, ampDb, yBottom, yTop);
                g.drawLine(x, rect.getCentreY(), x, y);
                path.lineTo(x, y);
            }

            g.setColour(lineColour);
            g.strokePath(path, strokeType);
            //g.restoreState();
        }
    }

private:
    void calculate(
        std::vector<double>& angles,
        std::vector<double>& amplitudes,
        std::vector<double>& phases)
    {
        auto size = angles.size();
        amplitudes.resize(size);
        phases.resize(size);

        for (size_t i = 0; i < size; i++)
        {
            amplitudes[i] = 1.0;
            phases[i] = 0.0;
        }

        double ampCoeff, phaseCoeff;
        for (auto pole : processor->filterState->poles)
        {
            for (size_t i = 0; i < size; i++)
            {
                calculateCoefficients(angles[i], pole, ampCoeff, phaseCoeff);
                for (auto j = 0; j < std::abs(pole->order); j++)
                {
                    amplitudes[i] *= ampCoeff; // later it turns to 1 / amplitudes[i]
                    phases[i] -= phaseCoeff;
                }
            }
        }

        const double eps = std::numeric_limits<double>::epsilon();
        for (size_t i = 0; i < size; i++)
            amplitudes[i] = 1.0 / std::max(amplitudes[i], eps);

        for (auto zero : processor->filterState->zeros)
        {
            for (size_t i = 0; i < size; i++)
            {
                calculateCoefficients(angles[i], zero, ampCoeff, phaseCoeff);
                for (auto j = 0; j < zero->order; j++)
                {
                    amplitudes[i] *= ampCoeff;
                    phases[i] += phaseCoeff;
                }
            }
        }
    }

    inline void calculateCoefficients(double angle, FilterRoot* root, double &ampCoeff, double& phaseCoeff)
    {
        std::complex<double> point(std::cos(angle), std::sin(angle));
        std::complex<double> v = root->value.get() - point;
        ampCoeff = std::abs(v);
        phaseCoeff = std::arg(v);
        if (!root->isReal())
        {
            std::complex<double> conj(root->value.re, -root->value.im);
            v = conj - point;
            ampCoeff *= std::abs(v);
            phaseCoeff += std::arg(v);
        }
    }

    const juce::Colour
        backgroundColor = juce::Colour(0x08, 0x0C, 0x1C),
        lineColour = juce::Colours::white,
        gridColour = juce::Colours::darkgrey;
    const juce::PathStrokeType strokeType{ 3.f };
    const int
        padding = 5,
        plotPaddingLeft = 55,
        plotPaddingRight = 45,
        plotPaddingTop = 40,
        plotPaddingBottom = 20;
    const int
        zoomButtonsSize = 20,
        logScaleButtonWidth = 60,
        textWidth = 40,
        textHeight = 10;
    const float
        minAmpDb = 6.f,
        maxAmpDb = 96.f,
        minFreq = 20.f;

    float ampDb;
    double sampleRate;

    AudioPluginAudioProcessor* processor;

    juce::TextButton zoomInButton, zoomOutButton, logScaleButton;
};

// NOTE(ry): I need to put this here so my editor doesn't screw with the style of this file
/* Local Variables: */
/* mode: c++ */
/* tab-width: 4 */
/* c-basic-offset: 4 */
/* indent-tabs-mode: t */
/* buffer-file-coding-system: undecided-unix */
/* End: */
