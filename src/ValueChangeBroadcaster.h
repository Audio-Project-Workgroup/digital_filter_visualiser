#pragma once
#include <juce_events/juce_events.h>

template <typename Type>
class ValueChangeBroadcaster: public juce::ChangeBroadcaster
{
public:
    ValueChangeBroadcaster(Type v)
        : value(v)
    {
    }

    Type get()
    {
        return value;
    }

    void set(Type v)
    {
        if (this->value != v)
        {
            this->value = v;
            sendChangeMessage();
        }
    }

private:
    Type value;
};
