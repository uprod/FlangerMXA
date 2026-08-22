# Product

<!-- impeccable:product-schema 1 -->

## Platform

desktop

Native JUCE audio plugin (AU/VST3/Standalone, macOS 11+). Sibling of the MXA suite; family design authority: `../PhaserMXA/DESIGN.md`; family product context: `../PhaserMXA/PRODUCT.md`.

## Product Purpose

A flanger: a short LFO-swept delay (Lagrange-interpolated) resummed with the dry signal to form a gliding comb filter, with bipolar feedback and selectable wet polarity (negative flanging).

## Capabilities and Constraints

- Exactly six parameters: `rate` (0.05–5 Hz), `depth` (sweep amount, up to +6 ms), `manual` (base delay 0.4–8 ms), `feedback` (-90..+90 %), `polarity` (Positive/Negative wet choice), `mix`.
- Delay = manual + depth × 6 ms × raised-cosine LFO; delay slewed ~2 ms against zipper; feedback taken from the delayed output back into the line (`FlangerEngine`).
- UI truth taps: atomic live delay (`uiDelayMs`) and LFO phase; static `lfo01For()` / `delayMsFor()` — the single source of truth for FIG. 1's comb (exact transfer (1-mix) + mix·pol·z/(1-fb·z) at the live delay, first-notch index at 1/(2D)) and FIG. 2's printed delay.
- Editor: Service Manual family sheet, 820×470, spot ink anis-green #C9E35D, DWG NO. MXA-FL-01.

## Brand Commitments

Inherits the family's: MXAudio, "BY MESCALINA" credit, one spot ink per sibling (Flanger = anis-green).

## Evidence on Hand

Working DSP (`Source/FlangerEngine.*`). No users/testimonials — nothing may be fabricated.
