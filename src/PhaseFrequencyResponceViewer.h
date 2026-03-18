#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "PluginProcessor.h"

class PhaseFrequencyResponseViewer final :
    public juce::Component,
    juce::ChangeListener
{
public:
    PhaseFrequencyResponseViewer(AudioPluginAudioProcessor* processor) :
        processor(processor),
        filterState(processor->filterState.get()),
        zoomInButton("+"),
        zoomOutButton("-"),
        ampDb(minAmpDb),
        sampleRate(processor->getSampleRate())
    {
        processor->addChangeListener(this);
        filterState->um->addChangeListener(this);
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

        addAndMakeVisible(zoomInButton);
        addAndMakeVisible(zoomOutButton);
    }

    ~PhaseFrequencyResponseViewer()
    {
        filterState->um->removeChangeListener(this);
        processor->removeChangeListener(this);
    }

    void changeListenerCallback(juce::ChangeBroadcaster* source) override
    {
        if (source == filterState->um)
            repaint();
        else if (source == processor)
        {
            sampleRate = processor->getSampleRate();
            repaint();
        }
    }

    void resized() override
    {
        zoomOutButton.setBounds(padding, padding, zoomButtonsSize, zoomButtonsSize);
        zoomInButton.setBounds(2 * padding + zoomButtonsSize, padding, zoomButtonsSize, zoomButtonsSize);
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getBounds();
        auto width = bounds.getWidth() - plotPaddingLeft - plotPaddingRight;
        auto height = bounds.getHeight() - plotPaddingTop - plotPaddingBottom;
        float xLeft = static_cast<float>(plotPaddingLeft);
        float xRight = static_cast<float>(xLeft + width);
        float yTop = static_cast<float>(plotPaddingTop);
        float yBottom = static_cast<float>(yTop + height);

        if (width < 2 || height < 2)
            return;

        std::vector<double> angles(static_cast<std::size_t>(width));
        std::vector<double> amplitudes, phases;

        const double angleStep = juce::MathConstants<double>::pi / width;
        for (int i = 0; i < width; i++)
            angles[i] = angleStep * i;

        calculate(angles, amplitudes, phases);

        for (int i = 0; i < width; i++)
            amplitudes[i] = juce::Decibels::gainToDecibels(amplitudes[i]);

        juce::Rectangle<int> rect(plotPaddingLeft, plotPaddingTop, width, height);
        g.fillAll(backgroundColor);
        g.setColour(gridColour);
        g.drawRect(rect, 1);
        
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

        if (sampleRate > 0)
        {
            g.drawText(
                "0",
                plotPaddingLeft - textWidth / 2,
                static_cast<int>(yBottom) + padding,
                textWidth,
                textHeight,
                juce::Justification::centred);
            g.drawText(
                juce::String(static_cast<int>(sampleRate / 2)), 
                static_cast<int>(xRight) - textWidth / 2,
                static_cast<int>(yBottom) + padding,
                textWidth,
                textHeight,
                juce::Justification::centred);
            g.drawText(
                " Hz",
                static_cast<int>(xRight) + textWidth / 2,
                static_cast<int>(yBottom) + padding,
                textWidth,
                textHeight,
                juce::Justification::centredLeft);
        }

        juce::ColourGradient gradient(
            juce::Colours::red.withAlpha(0.75f), 0.0f, rect.getY(),
            juce::Colours::green.withAlpha(0.75f), 0.0f, rect.getBottom(),
            false);
        gradient.addColour(0.5, juce::Colours::yellow.withAlpha(0.75f));

        {
            const juce::Graphics::ScopedSaveState state(g);
            g.reduceClipRegion(rect);
            
            g.setGradientFill(gradient);
            
            const float piFloat = juce::MathConstants<float>::pi;
            juce::Path path;
            auto x = juce::jmap(static_cast<float>(angles[0]), 0.f, piFloat, xLeft, xRight);
            auto y = juce::jmap(static_cast<float>(amplitudes[0]), -ampDb, ampDb, yBottom, yTop);
            g.drawLine(x, rect.getCentreY(), x, y);
            path.startNewSubPath(x, y);
            for (int i = 1; i < width; i++)
            {
                x = juce::jmap(static_cast<float>(angles[i]), 0.f, piFloat, xLeft, xRight);
                y = juce::jmap(static_cast<float>(amplitudes[i]), -ampDb, ampDb, yBottom, yTop);
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

        for (auto i = 0; i < size; i++)
        {
            amplitudes[i] = 1.0;
            phases[i] = 0.0;
        }

        double ampCoeff, phaseCoeff;
        for (auto pole : filterState->poles)
        {
            for (auto i = 0; i < size; i++)
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
        for (auto i = 0; i < size; i++)
            amplitudes[i] = 1.0 / std::max(amplitudes[i], eps);

        for (auto zero : filterState->zeros)
        {
            for (auto i = 0; i < size; i++)
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
        textWidth = 40,
        textHeight = 10;
    const float
        minAmpDb = 6.f,
        maxAmpDb = 96.f;

    float ampDb;
    double sampleRate;

    AudioPluginAudioProcessor* processor;
    FilterState* filterState;

    juce::TextButton zoomInButton, zoomOutButton;
};