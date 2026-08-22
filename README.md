# FlangerMXA

A flanger: a short LFO-swept delay (Lagrange-interpolated) resummed with the dry signal to form a gliding comb filter, with bipolar feedback and selectable wet polarity (negative flanging).

Audio plugin (AU / VST3 / Standalone) built with [JUCE](https://juce.com). Part of the [MXA plugin suite](https://mxaudio.mescalina.fr/). macOS 11+ and Windows — Windows builds (VST3 + Standalone) are available in [Releases](https://github.com/uprod/FlangerMXA/releases).

## Build

```sh
git clone --recurse-submodules https://github.com/uprod/FlangerMXA.git
cd FlangerMXA
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

If you already have the MXA suite checked out with a shared `../JUCE` folder, the submodule is optional — the build falls back to the sibling folder automatically.
