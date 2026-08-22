#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

namespace flangermxa
{

// FIG. 1 - Reponse en frequence, tracee comme la figure d'un manuel technique.
// Ce n'est pas une illustration : la courbe est la vraie fonction de transfert
// du peigne (retard + feedback + polarite + mix) evaluee au retard REEL que le
// LFO impose au moteur en ce moment meme : les encoches glissent avec le
// balayage. Le repaint est pilote par le Timer de l'editeur (~30 Hz).
class ResponsePlot : public juce::Component
{
public:
    explicit ResponsePlot (FlangerProcessor&);

    void paint (juce::Graphics&) override;

private:
    FlangerProcessor& processor;

    std::atomic<float>* manual   = nullptr;
    std::atomic<float>* depth    = nullptr;
    std::atomic<float>* feedback = nullptr;
    std::atomic<float>* polarity = nullptr;
    std::atomic<float>* mix      = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResponsePlot)
};

}
