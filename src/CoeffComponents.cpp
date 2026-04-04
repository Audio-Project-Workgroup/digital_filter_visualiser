#include "CoeffComponents.h"

#include <iostream>
#define COL_WIDTH 100 // TODO make this value relate to the global UI.

CoefficientsComponent::CoefficientsComponent(FilterState& state)
    : isExpanded(bool(DEFAULT_IS_EXPANDED))
    , filterState(state)
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

    filterState.treeRoot.addListener(this);
}

CoefficientsComponent::~CoefficientsComponent()
{
    // TODO properly deallocate every label is created with new. Actually all the labels.
}

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
    
    auto* label = static_cast<juce::Label*>(existing);
    if (!label)
    {
        // Create a new editable Label restricting user from invalid input

        label = new juce::Label();
        label->setEditable(true, true, false);

        // restrict input when editor apears
        label->onEditorShow = [label] {
            if (auto* editor = label->getCurrentTextEditor())
                editor->setInputRestrictions(0, "0123456789.-");
        };

        // update data when editing finishes
        label->onTextChange = [this, row, col, label]{
            double value = label->getText().getDoubleValue();
            if (col == 2)
                ffcoeffs[row] = value;
            else
                fbcoeffs[row] = value;
            
            // TODO: calculate roots through the coefficient2roots function.
            // TODO: notify other listeners about this change
        };
    }

    // update value prevernting dump data from appearing in case of a vector size mismatch
    double value;
    if (col == 2)
    {
        value = row < ffcoeffs.size() ? ffcoeffs[row] : 0;
    } 
    else if (col == 3)
    {
        value = row < fbcoeffs.size() ? fbcoeffs[row] : 0;
    }
    label->setText(juce::String(value), juce::dontSendNotification );
    return label;
}

void CoefficientsComponent::valueTreePropertyChanged (juce::ValueTree& node, const juce::Identifier& property)
{
    // 
    std::cout<<"valueTreePropertyChanged!"<<std::endl;
    if(property == IDs::ValueRe || property == IDs::ValueIm)
    {
    double valueRe = node.getProperty(IDs::ValueRe);
    double valueIm = node.getProperty(IDs::ValueIm);
    // updateBounds(c128(valueRe, valueIm));
    std::cout<< "valueRe "<<valueRe<<" valueIm " <<valueIm<< std::endl;
        updateCoeffTable();

    }
}

void CoefficientsComponent::valueTreeChildAdded (juce::ValueTree& node, juce::ValueTree& child)
{
    updateCoeffTable();
}

void CoefficientsComponent::valueTreeChildRemoved (juce::ValueTree& node, juce::ValueTree& child, int idx)
{   
    if (filterState.totalOrder==0) // if filter is cleaned out by erasing the last root
    {
        ffcoeffs.clear();
        fbcoeffs.clear();
        coeffTable.updateContent();
        return;
    }
    updateCoeffTable();
}

void CoefficientsComponent::updateCoeffTable(){
    
    filterState.zeros.size();
    filterState.poles.size();

	ffcoeffs = RootsToCoefficients::CalculatePolynomialCoefficientsFrom(
		filterState.zeros);
    fbcoeffs = RootsToCoefficients::CalculatePolynomialCoefficientsFrom(
		filterState.poles);
    
    // std::cout<<ffcoeffs.size()<<" ";
    // for (auto ff : ffcoeffs){
    //     std::cout<<ff<<" ";
    // }std::cout<<std::endl;

    // std::cout<<fbcoeffs.size()<<" --> ";
    // for (auto fb : fbcoeffs){
    //     std::cout<<fb<<" ";
    // }std::cout<<std::endl;

    coeffTable.updateContent();
}