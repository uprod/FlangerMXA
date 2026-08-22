#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

namespace flangermxa
{

// FIG. 2 - Le chemin du signal dessine comme dans le manuel : IN, sommateur de
// feedback, ligne a retard dont le temps imprime est le retard REEL du
// balayage, boucle de retour dont l'epaisseur EST le |feedback| (signe
// imprime), LFO avec point de phase reel, rails dry/wet ponderes par le mix,
// polarite du wet imprimee. La quantite est dessinee en geometrie : le schema
// est la valeur.
class SchematicDiagram : public juce::Component
{
public:
    explicit SchematicDiagram (FlangerProcessor&);

    void paint (juce::Graphics&) override;

private:
    FlangerProcessor& processor;

    std::atomic<float>* depth    = nullptr;
    std::atomic<float>* feedback = nullptr;
    std::atomic<float>* polarity = nullptr;
    std::atomic<float>* mix      = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SchematicDiagram)
};

}
