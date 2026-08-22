#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <cmath>

namespace flangermxa
{

// Coeur DSP du flanger. Le principe : le son est resomme avec une copie de
// lui-meme retardee de quelques millisecondes — les deux versions s'annulent
// a intervalles reguliers du spectre, creant un peigne d'encoches. Un LFO fait
// glisser ce retard (le "whoosh"), le feedback renvoie la sortie retardee dans
// la ligne pour creuser des resonances, et la polarite du wet inverse le
// peigne (flanging negatif, plus creux). L'interpolation Lagrange permet des
// retards fractionnaires, indispensable pour un balayage lisse.
class FlangerEngine
{
public:
    static constexpr int   kMaxCh    = 2;
    static constexpr float kSweepMs  = 6.0f;   // excursion max du balayage
    static constexpr float kManualMaxMs = 8.0f;

    FlangerEngine();

    void prepare (double sampleRate, int blockSize, int numChannels);
    void reset();

    void setRateHz (float hz);
    void setDepth (float amount01);
    void setManualMs (float ms);
    void setFeedback (float amount);       // -0.9 .. +0.9
    void setNegative (bool neg);           // polarite du wet
    void setMix (float amount01);

    // Traite le buffer en place (dry/wet compris).
    void process (juce::AudioBuffer<float>& buffer);

    // Course du LFO (0..1) pour une phase donnee : cosinus leve, depart au
    // retard manuel. Partagee avec l'UI : une seule source de verite.
    static float lfo01For (float phase01) noexcept
    {
        const float ph = phase01 - std::floor (phase01);
        return 0.5f - 0.5f * std::cos (ph * juce::MathConstants<float>::twoPi);
    }

    // Retard instantane pour une position de LFO donnee.
    static float delayMsFor (float manualMs, float depth01, float lfo01) noexcept
    {
        return manualMs + juce::jlimit (0.0f, 1.0f, depth01) * kSweepMs * lfo01;
    }

    // Retard reel (ms) et phase LFO reelle, publies pour l'UI.
    float getDelayMsLive() const noexcept { return uiDelayMs.load (std::memory_order_relaxed); }
    float getLfoPhase01() const noexcept  { return uiPhase.load (std::memory_order_relaxed); }

private:
    double sampleRate = 44100.0;
    int    numCh = 2;

    float rateHz   = 0.3f;
    float depth    = 0.6f;
    float manualMs = 1.2f;
    float feedback = 0.4f;
    bool  negative = false;
    float mix      = 0.5f;

    float lfoPhase   = 0.0f;   // phase du LFO, 0..1
    float delayState = 1.2f;   // retard lisse (ms), anti-zipper sur MANUAL/DEPTH
    float mixState   = 0.5f;
    float kSlew = 0.01f;       // coefficient un-pole (~2 ms), fixe dans prepare()
    float kSlow = 0.005f;      // idem (~10 ms) pour le mix

    float fbState[kMaxCh] { 0.0f, 0.0f };   // derniere sortie retardee, par canal

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLine { 1 << 15 };

    std::atomic<float> uiDelayMs { 1.2f };
    std::atomic<float> uiPhase   { 0.0f };
};

}
