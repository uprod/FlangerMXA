#include "FlangerEngine.h"

namespace flangermxa
{

FlangerEngine::FlangerEngine() = default;

void FlangerEngine::prepare (double newSampleRate, int blockSize, int numChannels)
{
    sampleRate = newSampleRate;
    numCh = juce::jlimit (1, kMaxCh, numChannels);

    juce::dsp::ProcessSpec spec { sampleRate,
                                  (juce::uint32) juce::jmax (1, blockSize),
                                  (juce::uint32) numCh };
    delayLine.prepare (spec);

    // Retard maximal = manuel max + excursion max, plus une marge de securite.
    const float maxDelaySamples = (kManualMaxMs + kSweepMs) * 0.001f * (float) sampleRate + 4.0f;
    delayLine.setMaximumDelayInSamples ((int) maxDelaySamples);

    kSlew = 1.0f - std::exp (-1.0f / (0.002f * (float) sampleRate));
    kSlow = 1.0f - std::exp (-1.0f / (0.010f * (float) sampleRate));

    reset();
}

void FlangerEngine::reset()
{
    delayLine.reset();
    lfoPhase   = 0.0f;
    delayState = manualMs;
    mixState   = mix;
    for (auto& f : fbState)
        f = 0.0f;
}

void FlangerEngine::setRateHz (float hz)     { rateHz   = juce::jlimit (0.02f, 8.0f, hz); }
void FlangerEngine::setDepth (float a)       { depth    = juce::jlimit (0.0f, 1.0f, a); }
void FlangerEngine::setManualMs (float ms)   { manualMs = juce::jlimit (0.2f, kManualMaxMs, ms); }
void FlangerEngine::setFeedback (float a)    { feedback = juce::jlimit (-0.9f, 0.9f, a); }
void FlangerEngine::setNegative (bool neg)   { negative = neg; }
void FlangerEngine::setMix (float a)         { mix      = juce::jlimit (0.0f, 1.0f, a); }

void FlangerEngine::process (juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int chs        = juce::jmin (numCh, buffer.getNumChannels());

    const float inc = rateHz / (float) sampleRate;   // avance de phase par echantillon
    const float pol = negative ? -1.0f : 1.0f;

    for (int n = 0; n < numSamples; ++n)
    {
        const float targetMs = delayMsFor (manualMs, depth, lfo01For (lfoPhase));
        delayState += kSlew * (targetMs - delayState);
        mixState   += kSlow * (mix - mixState);

        float delaySamples = delayState * 0.001f * (float) sampleRate;
        if (delaySamples < 1.0f) delaySamples = 1.0f;

        for (int ch = 0; ch < chs; ++ch)
        {
            const float x = buffer.getSample (ch, n);

            // Le feedback renvoie la derniere sortie retardee dans la ligne.
            delayLine.pushSample (ch, x + feedback * fbState[ch]);
            const float y = delayLine.popSample (ch, delaySamples);
            fbState[ch] = y;

            buffer.setSample (ch, n, (1.0f - mixState) * x + mixState * pol * y);
        }

        lfoPhase += inc;
        if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
    }

    uiDelayMs.store (delayState, std::memory_order_relaxed);   // pour FIG. 1 / FIG. 2
    uiPhase.store (lfoPhase, std::memory_order_relaxed);
}

}
