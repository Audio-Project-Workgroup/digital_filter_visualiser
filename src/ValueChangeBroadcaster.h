#pragma once
#include <juce_events/juce_events.h>

template <typename Type>
class ValueChangeBroadcaster: public juce::ChangeBroadcaster
{
public:
    ValueChangeBroadcaster(Type value)
        : value(value)
    {
    }

    Type get()
    {
        return value;
    }

    void set(Type value)
    {
        if (this->value != value)
        {
            this->value = value;
            sendChangeMessage();
        }
    }

private:
    Type value;
};