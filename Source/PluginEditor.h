#pragma once

/*  IMPECCABLE DIRECTION CONTRACT — seed 5bcea053 (roll: assigned)
    THESIS: The panel IS the signal path — a service-manual schematic read as
    the circuit you hear; refuses knobs-on-a-metal-plate.
    OWN-WORLD: Diazo film negative — dark drafting film #17140F, pale ink
    #E6DCC2, spot anis-green #C9E35D (one spot ink per MXA sibling). Routed
    Gothic drafting lettering + Courier Prime figures, double sheet border,
    title block, FIG. captions.
    STORY: A producer reads the schematic, watches the comb notches glide
    with the sweep in FIG. 1, and trusts every figure at a glance.
    FIRST VIEWPORT: Header + title block; FIG. 1 live comb response at the
    real swept delay full width; FIG. 2 signal path with the real delay time
    printed in the block, weighted feedback loop with its true sign, LFO
    phase dot, weighted DRY/WET rails with printed wet polarity; six
    schematic dials beneath.
    SIGNATURE: the glide — FIG. 1's notches, FIG. 2's printed delay and LFO
    dot on one 30 Hz clock, driven by the engine's real swept delay.
    FORM: Service Manual family template, adopted from PhaserMXA, seed 5bcea053.
    FINISH: unreviewed and undocumented is unfinished; this build ends with
    the finish review, the verdict, DESIGN.md, and every shipping raster
    carrying its provenance.
*/

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "ManualStyle.h"
#include "ResponsePlot.h"
#include "SchematicDiagram.h"

namespace flangermxa
{

class FlangerEditor : public juce::AudioProcessorEditor,
                      private juce::Timer
{
public:
    explicit FlangerEditor (FlangerProcessor& proc);
    ~FlangerEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent& e) override;

private:
    using APVTS   = juce::AudioProcessorValueTreeState;
    using SAttach = APVTS::SliderAttachment;

    struct Dial
    {
        juce::Slider slider;
        juce::Label  name;
        std::unique_ptr<SAttach> attachment;
    };

    void setupDial (Dial& d, const juce::String& labelText, const juce::String& paramID);
    void timerCallback() override;

    void drawSheetFrame (juce::Graphics& g);
    void drawHeader (juce::Graphics& g);

    FlangerProcessor& processor;

    ManualLookAndFeel lookAndFeel;
    juce::Image       filmTexture;

    ResponsePlot     plot;
    SchematicDiagram schematic;

    Dial rateDial, depthDial, manualDial, fbDial, polSwitch, mixDial;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FlangerEditor)
};

}
