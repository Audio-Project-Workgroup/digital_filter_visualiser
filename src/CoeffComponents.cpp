#include "CoeffComponents.h"
#include "RootsToCoefficients.h"
#include "CoefficientsToRoots.cpp"

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
    coeffTable.setHeader(std::make_unique<CoeffTableHeader>());
    juce::TableHeaderComponent& tbComp = coeffTable.getHeader();
    tbComp.addColumn("#", 1, int(COL_WIDTH/3));
    tbComp.addColumn("FF", 2, int(COL_WIDTH));
    tbComp.addColumn("FB", 3, int(COL_WIDTH));
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
	PROFILE_FUNCTION();

    // TODO adjust dims and position based on area or neighbor components --> should call something like getChildren() + getName() but seems fragile quering approach
    auto area = getLocalBounds();
    constexpr int button_height {30};
    //constexpr int button_width {150};
    //constexpr int width_besideExportButton {102};
    titleButton.setBounds(0, 0, getWidth(), button_height);
    constexpr int offsetFromBottom {35};
    coeffTable.setBounds(0, button_height, getWidth(), area.getHeight()-offsetFromBottom);
}

int CoefficientsComponent::getNumRows()
{
    return static_cast<int>(fbcoeffs.size());
}

void CoefficientsComponent::paintRowBackground(juce::Graphics& g, int row, int w, int h, bool rowIsSelected)
{
	PROFILE_FUNCTION();

    juce::ignoreUnused(row);
	juce::ignoreUnused(w);
	juce::ignoreUnused(h);

    // TODO fix line can t become unselected, but only when other line is selected
    if (rowIsSelected)
    {
        g.fillAll(juce::Colours::lightblue);
    }
}

void CoefficientsComponent::paintCell(juce::Graphics& g, int row, int col, int w, int h, bool)
{
	PROFILE_FUNCTION();

    if (row >= getNumRows()) return;

    g.setColour(juce::Colours::white);
    g.drawRect(0, 0, w, h);

    juce::String text;
    if (col == 1) text = juce::String(row);
    if (col == 2) text = juce::String(ffcoeffs[static_cast<size_t>(row)]);
    if (col == 3) text = juce::String(fbcoeffs[static_cast<size_t>(row)]);

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
		label->setColour(juce::Label::outlineColourId, juce::Colours::white);

        label->onEditorShow = [label] {
            // restrict input when editor apears
            if (auto* editor = label->getCurrentTextEditor())
                editor->setInputRestrictions(0, "0123456789.-");
        };

        // update data when editing finishes
        label->onTextChange = [this, row, col, label]{
            double value = label->getText().getDoubleValue();
            updateFilterStateOnCoefEdit(row, col, value);
        };
    }

    // update value prevernting dump data from appearing in case of a vector size mismatch
    double value;
    if (col == 2)
    {
        value = ffcoeffs[static_cast<size_t>(row)];
    }
    else // col == 3
    {
		jassert(col == 3);
        value = fbcoeffs[static_cast<size_t>(row)];
    }
    label->setText(juce::String(value), juce::dontSendNotification );
    return label;
}


void CoefficientsComponent::updateFilterStateOnCoefEdit(int row, int col, double value)
{
    // calculate roots through the coefficient2roots function && notify other listeners about this change

    // @TODO Clean print statements (after ensuring robust implementation)...

    if (col == 2)
        ffcoeffs[static_cast<size_t>(row)] = value;
    else if (col == 3)
        fbcoeffs[static_cast<size_t>(row)] = value;
    
    if (col == 2)
    {

        auto zeros = CoefficientsToRoots::GramSchmidt(this->ffcoeffs);

#ifdef DEBUG_C2R
        std::cout<<"Previous Zeros"<<std::endl;
        for (int i=0 ; i< processor->filterState->zeros.size(); i++)
        {
            auto *r = processor->filterState->zeros[i];
            std::cout<<"("<<r->value.re.get()<<","<< r->value.im.get()<<") ";
        }std::cout<<std::endl;
#endif

        std::cout<<"Removing Zeroes"<<std::endl;
        while (!processor->filterState->zeros.isEmpty())
        {
            auto *r = processor->filterState->zeros.getFirst();
            std::cout<<"\t("<<r->value.re.get()<<","<< r->value.im.get()<<") ";
            processor->filterState->remove(r);
        }
        std::cout<<std::endl;

        std::cout<<"Adding Zeros"<<std::endl;
        for (int i=0 ; i< zeros.size(); i++)
        {
            auto &r = zeros[i].first;
            auto &order = zeros[i].second;
            std::cout<<"\t("<<r.real()<<","<< r.imag()<<") ";
            processor->filterState->add(order, r);
        }
        std::cout<<std::endl;

#ifdef DEBUG_C2R
        std::cout<<"Final Zeros"<<std::endl;
        for (int i=0 ; i< processor->filterState->zeros.size(); i++)
        {
            auto *r = processor->filterState->zeros[i];

            std::cout<<"("<<r->value.re.get()<<","<< r->value.im.get()<<") ";
        }std::cout<<std::endl;
#endif
    }
    else if (col == 3)
    {
        auto poles =  CoefficientsToRoots::GramSchmidt(this->fbcoeffs);

#ifdef DEBUG_C2R
        std::cout<<"Previous Poles"<<std::endl;
        for (int i=0 ; i< processor->filterState->poles.size(); i++)
        {
            auto *r = processor->filterState->poles[i];
            std::cout<<"\t("<<r->value.re.get()<<","<< r->value.im.get()<<") ";
        }std::cout<<std::endl;
#endif
        std::cout<<"Adding Poles"<<std::endl;
        for (int i=0 ; i< poles.size(); i++)
        {
            
            auto &r = poles[i].first;
            auto &order = poles[i].second;
            processor->filterState->add(-order, r);
            std::cout<<"\t("<<r.real()<<","<< r.imag()<<") ";
        }
        std::cout<<std::endl;

        std::cout<<"Removing Poles"<<std::endl;

        auto isNewPole = [&](c128 val) -> std::pair<bool, int> {
            for (auto& [r, order] : poles) 
            {
                if (val.real() == r.real() && val.imag() == r.imag())
                    return {true, order};
            }
            return {false, 0};
        };
        
        size_t sz = processor->filterState->poles.size() - poles.size();
        for (int i=0 ; i< sz; i++)
        {
            auto *r = processor->filterState->poles.getFirst();
            std::cout<<"\t("<<r->value.re.get()<<","<< r->value.im.get()<<") ";
            c128 key(c128(r->value.re.get(),r->value.im.get()));
            auto [found, order] = isNewPole(key);

            if ( found )
            {
                r->order= order;
            }
            else
            {
                processor->filterState->remove(r);
            } 
        }
        std::cout<<std::endl;
#ifdef DEBUG_C2R
        std::cout<<"Final Poles"<<std::endl;
        for (int i=0 ; i< processor->filterState->poles.size(); i++)
        {
            auto *r = processor->filterState->poles[i];
            std::cout<<"\t("<<r->value.re.get()<<","<< r->value.im.get()<<") ";
        }std::cout<<std::endl;
#endif
    }
#ifdef DEBUG_C2R
    std::cout<<"Root order "<<processor->filterState->totalOrder <<" finiteZerosOrder "<< processor->filterState->finiteZerosOrder<<std::endl;
#endif
    
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
	juce::ignoreUnused(node);
	juce::ignoreUnused(child);

    updateCoeffTable();
}

void CoefficientsComponent::valueTreeChildRemoved (juce::ValueTree& node, juce::ValueTree& child, int idx)
{
	juce::ignoreUnused(node);
	juce::ignoreUnused(child);
	juce::ignoreUnused(idx);

    if (processor->filterState->totalOrder==0) // if filter is cleaned out by erasing the last root
    {
        ffcoeffs.clear();
        fbcoeffs.clear();
        coeffTable.updateContent();
        return;
    }
    updateCoeffTable();
}

void CoefficientsComponent::updateCoeffTable()
{
    fbcoeffs = RootsToCoefficients::CalculatePolynomialCoefficientsFrom(
        processor->filterState->poles);
    ffcoeffs = RootsToCoefficients::CalculatePolynomialCoefficientsFrom(
		processor->filterState->zeros, static_cast<int>(fbcoeffs.size()));
    coeffTable.updateContent();
}

// NOTE(ry): I need to put this here so my editor doesn't screw with the style of this file
/* Local Variables: */
/* mode: c++ */
/* tab-width: 4 */
/* c-basic-offset: 4 */
/* indent-tabs-mode: t */
/* buffer-file-coding-system: undecided-unix */
/* End: */
