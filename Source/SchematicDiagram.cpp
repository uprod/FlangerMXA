#include "SchematicDiagram.h"
#include "ManualStyle.h"
#include "FlangerEngine.h"

namespace flangermxa
{

namespace
{
    // Epaisseur de trait proportionnelle a une quantite 0..1 : la geometrie
    // porte la valeur, jamais la couleur seule.
    float weightFor (float amount01)
    {
        return 0.7f + 2.4f * juce::jlimit (0.0f, 1.0f, amount01);
    }

    void drawArrowHead (juce::Graphics& g, juce::Point<float> tip, juce::Point<float> dir, float size)
    {
        dir = dir / (dir.getDistanceFromOrigin() + 1.0e-6f);
        const juce::Point<float> n (-dir.y, dir.x);
        juce::Path p;
        p.addTriangle (tip, tip - dir * size + n * (size * 0.55f),
                             tip - dir * size - n * (size * 0.55f));
        g.fillPath (p);
    }

    void drawDashedLine (juce::Graphics& g, juce::Line<float> line, float thickness)
    {
        const float dashes[] = { 3.0f, 3.0f };
        g.drawDashedLine (line, dashes, 2, thickness);
    }

    // Etiquette imprimee qui interrompt le trait qu'elle chevauche : on pose
    // un cartouche couleur film derriere le texte, comme sur un vrai plan.
    void drawLabelOverLine (juce::Graphics& g, const juce::String& text,
                            juce::Rectangle<float> area, juce::Justification just)
    {
        const float tw = juce::GlyphArrangement::getStringWidth (fonts::lettering (9.0f), text);
        auto knockout = area.withSizeKeepingCentre (tw + 10.0f, area.getHeight());
        g.setColour (palette::film);
        g.fillRect (knockout);
        g.setFont (fonts::lettering (9.0f));
        g.setColour (palette::inkMid);
        g.drawText (text, area, just);
    }

    // Croix de sommateur dans un cercle (jonction "+" du schema).
    void drawSummingNode (juce::Graphics& g, juce::Point<float> c, float r)
    {
        g.setColour (palette::film);
        g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
        g.setColour (palette::ink);
        g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 1.1f);
        g.drawLine (c.x - r * 0.5f, c.y, c.x + r * 0.5f, c.y, 1.0f);
        g.drawLine (c.x, c.y - r * 0.5f, c.x, c.y + r * 0.5f, 1.0f);
    }

    // Rail pondere : epaisseur = quantite ; a zero il degenere en tirete fin.
    void drawWeightedLine (juce::Graphics& g, juce::Line<float> line, float amount01)
    {
        if (amount01 < 0.005f)
            drawDashedLine (g, line, 0.7f);
        else
            g.drawLine (line, weightFor (amount01));
    }
}

SchematicDiagram::SchematicDiagram (FlangerProcessor& proc)
    : processor (proc)
{
    auto& apvts = processor.getAPVTS();
    depth    = apvts.getRawParameterValue ("depth");
    feedback = apvts.getRawParameterValue ("feedback");
    polarity = apvts.getRawParameterValue ("polarity");
    mix      = apvts.getRawParameterValue ("mix");

    setInterceptsMouseClicks (false, false);
}

void SchematicDiagram::paint (juce::Graphics& g)
{
    const float w = (float) getWidth();
    auto caption  = getLocalBounds().toFloat().removeFromBottom (16.0f);

    const float depthV = depth->load();
    const float fbV    = feedback->load();
    const bool  neg    = juce::roundToInt (polarity->load()) == 1;
    const float mixV   = mix->load();

    const float delayMs = processor.getDelayMsLive();
    const float phase   = processor.getLfoPhase01();

    // Rangs horizontaux du schema.
    const float fbY  = 8.0f;    // boucle de retour
    const float wetY = 28.0f;   // rail principal (chemin retarde)
    const float lfoY = 58.0f;   // centre du cercle LFO
    const float dryY = 74.0f;   // rail dry

    // Colonnes.
    const float inX     = 12.0f;
    const float branchX = 36.0f;
    const float sumFbX  = 72.0f;
    const float blockX0 = 104.0f, blockX1 = 250.0f;   // bloc ligne a retard
    const float tapX    = w * 0.635f;
    const float mixX    = w * 0.86f;
    const float outX    = w - 16.0f;
    const float lfoCx   = (blockX0 + blockX1) * 0.5f;
    const float blockH  = 26.0f;

    const float wetW = weightFor (mixV);
    const float dryW = weightFor (1.0f - mixV);
    juce::ignoreUnused (wetW);

    // --- Rail d'entree et derivation dry ------------------------------------
    g.setColour (palette::ink);
    g.drawEllipse (inX - 3.0f, wetY - 3.0f, 6.0f, 6.0f, 1.1f);               // borne IN
    g.drawLine (inX + 3.0f, wetY, sumFbX - 8.0f, wetY, 1.2f);
    g.fillEllipse (branchX - 2.2f, wetY - 2.2f, 4.4f, 4.4f);                 // noeud de derivation

    g.setColour (palette::ink.withAlpha (0.9f));
    g.drawLine (branchX, wetY, branchX, dryY, dryW * 0.75f + 0.4f);          // descente dry
    g.drawLine (branchX, dryY, mixX, dryY, dryW);                            // rail dry
    g.drawLine (mixX, dryY, mixX, lfoY + 1.0f, dryW);                        // remontee vers le mix

    g.setFont (fonts::lettering (9.0f));
    g.setColour (palette::inkMid);
    g.drawText ("IN", juce::Rectangle<float> (24.0f, 10.0f).withPosition (inX - 8.0f, wetY - 17.0f),
                juce::Justification::centredLeft);
    g.drawText ("DRY", juce::Rectangle<float> (30.0f, 10.0f).withPosition (branchX + 8.0f, dryY - 13.0f),
                juce::Justification::centredLeft);

    // --- Sommateur de feedback ----------------------------------------------
    g.setColour (palette::ink);
    g.drawLine (sumFbX + 7.0f, wetY, blockX0, wetY, 1.2f);
    drawArrowHead (g, { blockX0, wetY }, { 1.0f, 0.0f }, 6.0f);
    drawSummingNode (g, { sumFbX, wetY }, 7.0f);

    // --- Bloc ligne a retard : le temps imprime est le retard reel -----------
    {
        const juce::Rectangle<float> block (blockX0, wetY - blockH * 0.5f,
                                            blockX1 - blockX0, blockH);
        g.setColour (palette::film);
        g.fillRect (block);
        g.setColour (palette::ink);
        g.drawRect (block, 1.2f);

        g.setFont (fonts::lettering (10.0f));
        g.drawText ("DELAY LINE", block.withTrimmedBottom (9.0f), juce::Justification::centred);

        g.setFont (fonts::mono (8.0f));
        g.setColour (palette::inkMid);
        g.drawText (juce::String (delayMs, 2) + " ms",
                    block.withTrimmedTop (12.0f), juce::Justification::centred);
    }

    g.setColour (palette::ink);
    g.drawLine (blockX1, wetY, tapX, wetY, 1.2f);

    // --- Boucle de feedback : l'epaisseur EST le |feedback|, signe imprime ----
    g.fillEllipse (tapX - 2.2f, wetY - 2.2f, 4.4f, 4.4f);                    // noeud de reprise
    {
        const float fbAmt = std::abs (fbV) / 0.9f;
        g.setColour (palette::ink);
        if (fbAmt < 0.005f)
        {
            drawDashedLine (g, { { tapX, wetY }, { tapX, fbY } }, 0.7f);
            drawDashedLine (g, { { tapX, fbY }, { sumFbX, fbY } }, 0.7f);
            drawDashedLine (g, { { sumFbX, fbY }, { sumFbX, wetY - 7.0f } }, 0.7f);
        }
        else
        {
            const float fbW = weightFor (fbAmt);
            g.drawLine (tapX, wetY, tapX, fbY, fbW);
            g.drawLine (tapX, fbY, sumFbX, fbY, fbW);
            g.drawLine (sumFbX, fbY, sumFbX, wetY - 7.0f, fbW);
        }
        drawArrowHead (g, { sumFbX, wetY - 7.0f }, { 0.0f, 1.0f }, 6.0f);

        // L'etiquette interrompt le trait de la boucle : le vrai signe imprime.
        const juce::String fbText = "FEEDBACK "
            + juce::String (fbV >= 0.0005f ? "+" : "")
            + juce::String (juce::roundToInt (fbV * 100.0f)) + " %";
        drawLabelOverLine (g, fbText,
                           juce::Rectangle<float> (130.0f, 10.0f)
                               .withCentre ({ (blockX1 + tapX) * 0.5f, fbY }),
                           juce::Justification::centred);
    }

    // --- Rail wet vers le sommateur de mix : la polarite est imprimee ---------
    g.setColour (palette::ink.withAlpha (0.9f));
    drawWeightedLine (g, { { tapX, wetY }, { mixX, wetY } }, mixV);
    drawWeightedLine (g, { { mixX, wetY }, { mixX, lfoY - 1.0f } }, mixV);
    g.setFont (fonts::lettering (9.0f));
    g.setColour (palette::inkMid);
    g.drawText (neg ? "WET (-)" : "WET (+)",
                juce::Rectangle<float> (50.0f, 10.0f).withPosition (mixX - 64.0f, wetY - 13.0f),
                juce::Justification::centredRight);

    drawSummingNode (g, { mixX, lfoY }, 8.0f);
    g.drawText ("MIX", juce::Rectangle<float> (30.0f, 10.0f).withPosition (mixX + 12.0f, lfoY - 20.0f),
                juce::Justification::centredLeft);

    // --- Sortie --------------------------------------------------------------
    g.setColour (palette::ink);
    g.drawLine (mixX + 8.0f, lfoY, outX - 3.0f, lfoY, 1.4f);
    g.fillEllipse (outX - 3.0f, lfoY - 3.0f, 6.0f, 6.0f);                    // borne OUT
    g.setColour (palette::inkMid);
    g.drawText ("OUT", juce::Rectangle<float> (28.0f, 10.0f).withPosition (outX - 24.0f, lfoY - 17.0f),
                juce::Justification::centredRight);

    // --- LFO : point de phase reel, ligne de modulation = depth ---------------
    {
        const float r = 10.0f;

        g.setColour (palette::spot.withAlpha (0.6f));
        drawDashedLine (g, { { lfoCx, lfoY - r }, { lfoCx, wetY + blockH * 0.5f } },
                        depthV < 0.005f ? 0.7f : weightFor (depthV));
        g.setColour (palette::spot);
        drawArrowHead (g, { lfoCx, wetY + blockH * 0.5f }, { 0.0f, -1.0f }, 5.0f);

        // Rappel de plan : la ligne porte son nom, reliee par une amorce pointee.
        {
            const juce::Point<float> tip (lfoCx, (lfoY - r + wetY + blockH * 0.5f) * 0.5f);
            g.setColour (palette::inkMid);
            g.drawLine (tip.x, tip.y, tip.x + 16.0f, tip.y, 0.7f);
            g.fillEllipse (tip.x - 1.6f, tip.y - 1.6f, 3.2f, 3.2f);
            g.setFont (fonts::lettering (9.0f));
            g.drawText ("MODULATION",
                        juce::Rectangle<float> (90.0f, 10.0f).withPosition (tip.x + 20.0f, tip.y - 5.0f),
                        juce::Justification::centredLeft);
        }

        g.setColour (palette::film);
        g.fillEllipse (lfoCx - r, lfoY - r, r * 2.0f, r * 2.0f);
        g.setColour (palette::ink);
        g.drawEllipse (lfoCx - r, lfoY - r, r * 2.0f, r * 2.0f, 1.1f);

        // Un cycle de la course du LFO (cosinus leve) dans le cercle.
        juce::Path glyph;
        for (int i = 0; i <= 24; ++i)
        {
            const float t = (float) i / 24.0f;
            const float sx = lfoCx - 6.5f + 13.0f * t;
            const float sy = lfoY + 4.5f - 9.0f * FlangerEngine::lfo01For (t);
            if (i == 0) glyph.startNewSubPath (sx, sy);
            else        glyph.lineTo (sx, sy);
        }
        g.setColour (palette::inkMid);
        g.strokePath (glyph, juce::PathStrokeType (1.0f));

        // Point de phase reel.
        {
            const float ph = phase - std::floor (phase);
            const float dx = lfoCx - 6.5f + 13.0f * ph;
            const float dy = lfoY + 4.5f - 9.0f * FlangerEngine::lfo01For (ph);
            g.setColour (palette::spot);
            g.fillEllipse (dx - 2.2f, dy - 2.2f, 4.4f, 4.4f);
        }

        g.setFont (fonts::lettering (9.0f));
        g.setColour (palette::inkMid);
        g.drawText ("LFO",
                    juce::Rectangle<float> (30.0f, 10.0f).withPosition (lfoCx - r - 36.0f, lfoY - 5.0f),
                    juce::Justification::centredRight);
    }

    // --- Legende de figure ----------------------------------------------------
    const juce::String cap = juce::String ("FIG. 2 - SIGNAL PATH, SWEPT-DELAY COMB, ")
        + (neg ? "NEGATIVE" : "POSITIVE") + " WET";
    g.setFont (fonts::lettering (10.0f));
    g.setColour (palette::inkMid);
    g.drawText (cap, caption.withTrimmedTop (4.0f), juce::Justification::bottomLeft);

    const float capW = juce::GlyphArrangement::getStringWidth (fonts::lettering (10.0f), cap);
    g.setColour (palette::inkFaint);
    g.drawHorizontalLine ((int) (caption.getBottom() - 3.0f),
                          caption.getX() + capW + 10.0f, caption.getRight());
}

}
