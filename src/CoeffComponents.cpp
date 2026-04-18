#include "CoeffComponents.h"

#include <iostream>
#define COL_WIDTH 100 // TODO make this value relate to the global UI.

CoefficientsComponent::CoefficientsComponent(AudioPluginAudioProcessor* p)
    : isExpanded(bool(DEFAULT_IS_EXPANDED))
    , processor(p)
{
    titleButton.setButtonText(TABLE_TITLE);
    titleButton.onClick = [this]{ toggleCollapseExpand(); };
    addAndMakeVisible(titleButton);

    //setupCoefficient's table
    coeffTable.setModel(this); // pass ownership to the class.
    addAndMakeVisible(coeffTable);
    juce::TableHeaderComponent& tbComp = coeffTable.getHeader();
    tbComp.addColumn("#", 1, int(COL_WIDTH/3)); // @TODO create desc on hover --> Delay
    tbComp.addColumn("FF", 2, int(COL_WIDTH)); // @TODO create desc on hover --> FeedForward
    tbComp.addColumn("FB", 3, int(COL_WIDTH)); // @TODO create desc on hover --> Feedback
    tbComp.setStretchToFitActive(true); // expand columns to fill the entire width of the component
    coeffTable.setVisible(isExpanded);

    processor->filterState->addListener(this);
    processor->filterState->syncListener(this);
}

// Note:    Apart from any other reasons that may occur throughout future dev updates,..
//          ..in case that the labels created at refreshComponentForCell have to be properly deallocated,..
//          ..(See JUCE doc: https://docs.juce.com/master/classjuce_1_1TableListBoxModel.html#a873b2b84429f026ea046528af1f4fe81)..
//          ..the destructor should be defined, and consequently align explicitly to the rule of 5.
// CoefficientsComponent::~CoefficientsComponent() {}

void CoefficientsComponent::toggleCollapseExpand()
{
    isExpanded = !isExpanded;
    coeffTable.setVisible(isExpanded);
}

void CoefficientsComponent::resized()
{   
    // TODO adjust dims and position based on area or neighbor components
    auto area = getLocalBounds();
    constexpr int button_height {30}; // TODO can we retrieve this value?
    titleButton.setBounds(102, 0, 150, button_height);
    coeffTable.setBounds(102, button_height, 150, area.getHeight()-10);
}


int CoefficientsComponent::getNumRows() 
{
    return std::max(
        static_cast<int>(ffcoeffs.size()), 
        static_cast<int> (fbcoeffs.size())
    );
}

void CoefficientsComponent::paintRowBackground(juce::Graphics& g, int row, int w, int h, bool rowIsSelected)
{
    // TODO fix line can t become unselected, but only when other line is selected
    if (rowIsSelected)
    {
        g.fillAll(juce::Colours::lightblue);
    }
}

void CoefficientsComponent::paintCell(juce::Graphics& g, int row, int col, int w, int h, bool)
{
    if (row >= getNumRows()) return;

    g.setColour(juce::Colours::white);
    g.drawRect(0, 0, w, h);

    juce::String text;
    if (col == 1) text = juce::String(row);
    if (col == 2) text = juce::String(ffcoeffs[row]);
    if (col == 3) text = juce::String(fbcoeffs[row]);

    g.drawText(text, 0, 0, w, h, juce::Justification::centredLeft);
}

juce::Component* CoefficientsComponent::refreshComponentForCell(int row, int col, bool, juce::Component* existing)
{
    
    if (col !=2 && col !=3 )
        return nullptr;
    
    // creating new cells
    auto* label = static_cast<juce::Label*>(existing);
    if (!label)
    {
        // Create a new editable Label restricting user from invalid input

        label = new juce::Label();
        label->setEditable(true, true, false);

        label->onEditorShow = [label] {
            // restrict input when editor apears
            if (auto* editor = label->getCurrentTextEditor())
                editor->setInputRestrictions(0, "0123456789.-");
        };

        // update data when editing finishes
        label->onTextChange = [this, row, col, label]{
            double value = label->getText().getDoubleValue();
            if (col == 2 && row < ffcoeffs.size() )
                ffcoeffs[row] = value;
            else if ( col == 3 && row < fbcoeffs.size() )
                fbcoeffs[row] = value;
            
            // TODO: calculate roots through the coefficient2roots function.
            // TODO: notify other listeners about this change
        };
    }

    // update value prevernting dump data from appearing in case of a vector size mismatch
    double value;
    constexpr int defaultCoeffValue {0};
    if (col == 2)
    {
        value = row < ffcoeffs.size() ? ffcoeffs[row] : defaultCoeffValue;
    } 
    else if (col == 3)
    {
        value = row < fbcoeffs.size() ? fbcoeffs[row] : defaultCoeffValue;
    }
    label->setText(juce::String(value), juce::dontSendNotification );
    return label;
}

void CoefficientsComponent::valueTreePropertyChanged (juce::ValueTree& node, const juce::Identifier& property)
{
    if(property == IDs::ValueRe || property == IDs::ValueIm)    // when dragging roots
    {
        updateCoeffTable();
    }
    else if (property == IDs::Order)    // when changing order of a root
    {
        int order = node.getProperty(IDs::Order);
        if(order != 0) 
        {
            updateCoeffTable();
        }
    }
}

void CoefficientsComponent::valueTreeChildAdded (juce::ValueTree& node, juce::ValueTree& child)
{
    updateCoeffTable();
}

void CoefficientsComponent::valueTreeChildRemoved (juce::ValueTree& node, juce::ValueTree& child, int idx)
{   
    if (processor->filterState->totalOrder==0) // if filter is cleaned out by erasing the last root
    {
        ffcoeffs.clear();
        fbcoeffs.clear();
        coeffTable.updateContent();
        return;
    }
    updateCoeffTable();
}

void CoefficientsComponent::updateCoeffTable(){
    
	ffcoeffs = RootsToCoefficients::CalculatePolynomialCoefficientsFrom(
		processor->filterState->zeros);
    fbcoeffs = RootsToCoefficients::CalculatePolynomialCoefficientsFrom(
		processor->filterState->poles);

    // TODO requires optimization -  reversing should be done in-place
    std::reverse(ffcoeffs.begin(), ffcoeffs.end());
    std::reverse(fbcoeffs.begin(), fbcoeffs.end());

    coeffTable.updateContent();
}