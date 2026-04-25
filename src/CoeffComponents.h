#pragma once

#include "FilterState.h"
#include "RootsToCoefficients.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <string>

#define TABLE_TITLE "Coefficients"
#define DEFAULT_IS_EXPANDED 0

class CoefficientsComponent final
    : public juce::Component
    , private juce::TableListBoxModel
    , private juce::ValueTree::Listener
{
    public:
        CoefficientsComponent(AudioPluginAudioProcessor*);

        void resized() override;

    private:
        std::vector<double> ffcoeffs;
        std::vector<double> fbcoeffs;
        juce::TextButton titleButton;
        juce::TableListBox coeffTable;
        bool isExpanded;
        AudioPluginAudioProcessor *processor;
        juce::TooltipWindow tooltipWindow; 

        void toggleCollapseExpand();
        void updateCoeffTable();
        
        // override juce::Component
        // void paint(juce::Graphics &g) override; // TODO is this trully needed?

        // override juce::TableListBox 
        int getNumRows() override;
        void paintRowBackground(juce::Graphics&, int , int, int, bool) override;
        void paintCell(juce::Graphics&, int , int, int, int, bool) override; // method for updating columns
        juce::Component* refreshComponentForCell(int, int, bool, juce::Component*) override; // method for creating/updating single cells

        // override juce::ValueTree::Listener 
        void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override;
        void valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&) override;
        void valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree&, int) override;
        // will be used for single cell edits
        // void sendPropertyChangeMessage (const Identifier& property);         

        // provide tooltips for Table headers on hover
        class CoeffTableHeader 
            : public juce::TableHeaderComponent
            , public juce::TooltipClient
        {
            public:
                juce::String getTooltip() override
                {
                    auto pos = getMouseXYRelative();
                    const int idx = getColumnIdAtX(pos.x);
                    return (idx > 0 && idx <= (int)tooltips.size()) ? tooltips[idx-1] : "";
                }
            private:
                std::array<juce::String, 3> tooltips{"Delay", "FeedForward", "FeedBack"};
        };
};