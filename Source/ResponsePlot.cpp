#include "ResponsePlot.h"
#include "ManualStyle.h"
#include "FlangerEngine.h"

#include <complex>

namespace flangermxa
{

namespace
{
    constexpr float kFreqMin = 20.0f;
    constexpr float kFreqMax = 20000.0f;
    constexpr float kDbTop   = 18.0f;
    constexpr float kDbBot   = -42.0f;
    constexpr int   kPoints  = 240;

    float xForFreq (juce::Rectangle<float> r, float f)
    {
        const float t = std::log (f / kFreqMin) / std::log (kFreqMax / kFreqMin);
        return r.getX() + t * r.getWidth();
    }

    float yForDb (juce::Rectangle<float> r, float db)
    {
        const float t = (kDbTop - db) / (kDbTop - kDbBot);
        return r.getY() + t * r.getHeight();
    }

    float freqForX (juce::Rectangle<float> r, float x)
    {
        const float t = (x - r.getX()) / r.getWidth();
        return kFreqMin * std::pow (kFreqMax / kFreqMin, t);
    }

    juce::String freqLabel (float f)
    {
        if (f >= 1000.0f)
            return juce::String (f / 1000.0f, 0) + "k";
        return juce::String ((int) f);
    }

    // Module de la reponse totale : (1-mix) + mix x pol x z / (1 - fb x z),
    // z = e^(-j 2 pi f D) — le peigne exact du moteur au retard D courant.
    float responseDb (float freqHz, float delaySec, float fb, float pol, float mixAmt)
    {
        const float w = juce::MathConstants<float>::twoPi * freqHz * delaySec;
        const std::complex<float> z = std::polar (1.0f, -w);
        const std::complex<float> wet = z / (1.0f - fb * z);
        const std::complex<float> total = (1.0f - mixAmt) + mixAmt * pol * wet;
        return 20.0f * std::log10 (std::abs (total) + 1.0e-9f);
    }
}

ResponsePlot::ResponsePlot (FlangerProcessor& proc)
    : processor (proc)
{
    auto& apvts = processor.getAPVTS();
    manual   = apvts.getRawParameterValue ("manual");
    depth    = apvts.getRawParameterValue ("depth");
    feedback = apvts.getRawParameterValue ("feedback");
    polarity = apvts.getRawParameterValue ("polarity");
    mix      = apvts.getRawParameterValue ("mix");

    setInterceptsMouseClicks (false, false);
}

void ResponsePlot::paint (juce::Graphics& g)
{
    auto full = getLocalBounds().toFloat();
    auto caption = full.removeFromBottom (16.0f);
    auto box = full;

    const float manualV = manual->load();
    const float fbV     = feedback->load();
    const float pol     = juce::roundToInt (polarity->load()) == 1 ? -1.0f : 1.0f;
    const float mixV    = mix->load();

    float delayMs = processor.getDelayMsLive();
    if (delayMs <= 0.0f)
        delayMs = manualV;
    const float delaySec = delayMs * 0.001f;

    // --- Grille ------------------------------------------------------------
    g.setColour (palette::inkFaint);
    static const float gridFreqs[] = { 50.0f, 100.0f, 200.0f, 500.0f,
                                       1000.0f, 2000.0f, 5000.0f, 10000.0f };
    for (const float f : gridFreqs)
        g.drawVerticalLine ((int) xForFreq (box, f), box.getY() + 1.0f, box.getBottom() - 1.0f);

    static const float gridDbs[] = { 12.0f, 0.0f, -12.0f, -24.0f, -36.0f };
    for (const float db : gridDbs)
    {
        g.setColour (db == 0.0f ? palette::inkMid.withAlpha (0.65f) : palette::inkFaint);
        g.drawHorizontalLine ((int) yForDb (box, db), box.getX() + 1.0f, box.getRight() - 1.0f);
    }

    // Repere de la premiere encoche du peigne positif : f = 1/(2D).
    {
        const float fNotch = 1.0f / (2.0f * delaySec);
        if (fNotch >= kFreqMin && fNotch <= kFreqMax)
        {
            const float sx = xForFreq (box, fNotch);
            g.setColour (palette::spot.withAlpha (0.30f));
            g.drawVerticalLine ((int) sx, box.getY() + 1.0f, box.getBottom() - 1.0f);

            juce::Path idx;   // petit index triangulaire en haut
            idx.addTriangle (sx - 3.5f, box.getY() + 1.0f, sx + 3.5f, box.getY() + 1.0f,
                             sx, box.getY() + 7.0f);
            g.setColour (palette::spot);
            g.fillPath (idx);
        }
    }

    // --- Courbe ------------------------------------------------------------
    {
        juce::Path p;
        for (int i = 0; i < kPoints; ++i)
        {
            const float px = box.getX() + (float) i / (float) (kPoints - 1) * box.getWidth();
            const float f  = freqForX (box, px);
            const float db = juce::jlimit (kDbBot, kDbTop,
                                           responseDb (f, delaySec, fbV, pol, mixV));
            const float py = yForDb (box, db);
            if (i == 0) p.startNewSubPath (px, py);
            else        p.lineTo (px, py);
        }
        g.setColour (palette::spot);
        g.strokePath (p, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved));
    }

    // --- Echelles ----------------------------------------------------------
    // Chaque chiffre est pose sur un cartouche film : ni le grain ni la grille
    // ne peuvent corrompre une valeur que le regard doit pouvoir croire.
    auto drawFigure = [&g] (const juce::String& text, juce::Point<float> anchor,
                            juce::Justification just)
    {
        const auto font = fonts::mono (9.0f);
        const float tw  = juce::GlyphArrangement::getStringWidth (font, text);
        auto area = juce::Rectangle<float> (tw + 6.0f, 11.0f).withCentre (anchor);
        if (just.testFlags (juce::Justification::left))
            area.setX (anchor.x - 3.0f);
        else if (just.testFlags (juce::Justification::right))
            area.setX (anchor.x - tw - 3.0f);

        g.setColour (palette::film);
        g.fillRect (area);
        g.setFont (font);
        g.setColour (palette::inkMid);
        g.drawText (text, area, juce::Justification::centred);
    };

    for (const float f : gridFreqs)
        drawFigure (freqLabel (f), { xForFreq (box, f), box.getBottom() - 8.0f },
                    juce::Justification::centred);

    for (const float db : gridDbs)
        drawFigure ((db > 0.0f ? "+" : "") + juce::String ((int) db),
                    { box.getX() + 6.0f, yForDb (box, db) - 6.0f },
                    juce::Justification::left);

    // Designation des unites, une fois par echelle, convention de plan.
    drawFigure ("dB", { box.getX() + 6.0f, box.getY() + 10.0f }, juce::Justification::left);
    drawFigure ("Hz", { box.getRight() - 6.0f, box.getBottom() - 8.0f }, juce::Justification::right);

    // Tally du retard reel, en machine a ecrire : "dly  3.42 ms".
    {
        auto tally = juce::Rectangle<float> (120.0f, 12.0f)
                         .withPosition (box.getRight() - 126.0f, box.getY() + 6.0f);
        g.setColour (palette::film);
        g.fillRect (tally.expanded (3.0f, 1.0f));
        g.setFont (fonts::mono (10.0f));
        g.setColour (palette::inkMid);
        g.drawText ("dly", tally.removeFromLeft (30.0f), juce::Justification::centredLeft);
        g.setColour (palette::ink);
        g.drawText (juce::String (delayMs, 2) + " ms", tally, juce::Justification::centredLeft);
    }

    // --- Cadre + legende de figure ------------------------------------------
    g.setColour (palette::ink);
    g.drawRect (box, 1.0f);

    const juce::String cap = "FIG. 1 - FREQUENCY RESPONSE, COMB AT LIVE DELAY";
    g.setFont (fonts::lettering (10.0f));
    g.setColour (palette::inkMid);
    g.drawText (cap, caption.withTrimmedTop (4.0f), juce::Justification::bottomLeft);

    const float capW = juce::GlyphArrangement::getStringWidth (fonts::lettering (10.0f), cap);
    g.setColour (palette::inkFaint);
    g.drawHorizontalLine ((int) (caption.getBottom() - 3.0f),
                          caption.getX() + capW + 10.0f, caption.getRight());
}

}
