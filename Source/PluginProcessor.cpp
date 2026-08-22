#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace flangermxa
{

namespace IDs
{
    constexpr auto rate     = "rate";
    constexpr auto depth    = "depth";
    constexpr auto manual   = "manual";
    constexpr auto feedback = "feedback";
    constexpr auto polarity = "polarity";
    constexpr auto mix      = "mix";
}

juce::AudioProcessorValueTreeState::ParameterLayout FlangerProcessor::createLayout()
{
    using P = juce::AudioParameterFloat;

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Affichages "tally" a la machine a ecrire, aussi bien dans l'editeur que
    // dans les lignes d'automation de l'hote.
    const auto hzAttr = juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction ([] (float v, int) { return juce::String (v, 2) + " Hz"; })
        .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue(); });

    const auto pctAttr = juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; })
        .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue() / 100.0f; });

    const auto signedPctAttr = juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction ([] (float v, int)
            { return juce::String (v >= 0.0005f ? "+" : "")
                     + juce::String (juce::roundToInt (v * 100.0f)) + " %"; })
        .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue() / 100.0f; });

    const auto msAttr = juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction ([] (float v, int) { return juce::String (v, 2) + " ms"; })
        .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue(); });

    // Vitesse du balayage. Skew < 1 = plus de finesse dans les vitesses lentes.
    params.push_back (std::make_unique<P> (juce::ParameterID { IDs::rate, 1 },
        "Rate", juce::NormalisableRange<float> (0.05f, 5.0f, 0.01f, 0.5f), 0.3f, hzAttr));

    params.push_back (std::make_unique<P> (juce::ParameterID { IDs::depth, 1 },
        "Depth", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.6f, pctAttr));

    // Retard de base : la position "manuelle" de la lame du flanger.
    params.push_back (std::make_unique<P> (juce::ParameterID { IDs::manual, 1 },
        "Manual", juce::NormalisableRange<float> (0.4f, 8.0f, 0.01f, 0.5f), 1.2f, msAttr));

    // Feedback bipolaire : negatif = resonances aux creux plutot qu'aux pics.
    params.push_back (std::make_unique<P> (juce::ParameterID { IDs::feedback, 1 },
        "Feedback", juce::NormalisableRange<float> (-0.9f, 0.9f, 0.001f), 0.4f, signedPctAttr));

    // Polarite du wet : NEG inverse le peigne (flanging negatif, plus creux).
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { IDs::polarity, 1 }, "Polarity",
        juce::StringArray { "Positive", "Negative" }, 0));

    params.push_back (std::make_unique<P> (juce::ParameterID { IDs::mix, 1 },
        "Mix", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f, pctAttr));

    return { params.begin(), params.end() };
}

FlangerProcessor::FlangerProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
}

FlangerProcessor::~FlangerProcessor() = default;

void FlangerProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    engine.reset();
}

void FlangerProcessor::releaseResources()
{
    engine.reset();
}

bool FlangerProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& main = layouts.getMainOutputChannelSet();
    if (main != juce::AudioChannelSet::mono() && main != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == main;
}

void FlangerProcessor::pushParameterUpdatesToEngine()
{
    engine.setRateHz   (apvts.getRawParameterValue (IDs::rate)->load());
    engine.setDepth    (apvts.getRawParameterValue (IDs::depth)->load());
    engine.setManualMs (apvts.getRawParameterValue (IDs::manual)->load());
    engine.setFeedback (apvts.getRawParameterValue (IDs::feedback)->load());
    engine.setNegative (juce::roundToInt (apvts.getRawParameterValue (IDs::polarity)->load()) == 1);
    engine.setMix      (apvts.getRawParameterValue (IDs::mix)->load());
}

void FlangerProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    pushParameterUpdatesToEngine();

    // Le flanger agit en place, dry/wet compris : pas de copie de travail.
    engine.process (buffer);
}

juce::AudioProcessorEditor* FlangerProcessor::createEditor()
{
    return new FlangerEditor (*this);
}

void FlangerProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void FlangerProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

}

// Point d'entree du plugin JUCE — doit etre au niveau global.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new flangermxa::FlangerProcessor();
}
