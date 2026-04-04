#pragma once

#include "FilterState.h"
#include "RootsToCoefficients.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <string>

#define TABLE_TITLE "Coefficients"
#define DEFAULT_IS_EXPANDED 0

// https://docs.juce.com/master/classjuce_1_1Label.html
// https://forum.juce.com/t/label-editable-partly/32878

class CoefficientsComponent final
    : public juce::Component
    , private juce::TableListBoxModel
    , private juce::ValueTree::Listener
{
    public:
        CoefficientsComponent(FilterState&);
        ~CoefficientsComponent();

        void resized() override;    // needed?

    private:
        std::vector<double> ffcoeffs;
        std::vector<double> fbcoeffs;
        juce::TextButton titleButton;
        juce::TableListBox coeffTable;
        bool isExpanded;
        FilterState& filterState; 

        void toggleCollapseExpand();
        void updateCoeffTable();
        
        // override juce::Component
        // void paint(juce::Graphics &g) override; // TODO is this trully needed?

        // override juce::TableListBox 
            // pure virtual methods
        int getNumRows() override;
        void paintRowBackground(juce::Graphics&, int , int, int, bool) override;
        void paintCell(juce::Graphics&, int , int, int, int, bool) override; // method for updating columns
        juce::Component* refreshComponentForCell(int, int, bool, juce::Component*) override; // method for updating single cells

        // override juce::ValueTree::Listener 
        void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override;
        void valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&) override;
        void valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree&, int) override;
        // will be used for single cell edits
        // void sendPropertyChangeMessage (const Identifier& property); 
        // may be useful
        // virtual void valueTreeChildOrderChanged (ValueTree& parentTreeWhoseChildrenHaveMoved, int oldIndex, int newIndex);
        // virtual void valueTreeParentChanged (ValueTree& treeWhoseParentHasChanged);
            // void removeListener (Listener* listener);
            // ValueTree& setPropertyExcludingListener (Listener* listenerToExclude, const Identifier& name, const var& newValue, UndoManager* undoManager);
        
};