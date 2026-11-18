// Rotary Prism engine: plain C++17, no JUCE. Stereo in, stereo out, zero latency.
//
//   in -> mono sum -> 2x upsample -> RotorCore (preamp, crossover, rotors, mics/binaural, room)
//      -> 2x downsample -> wet limiter (audio-rate path) -> Mix with dry -> Output
//
// The rotor core always runs at twice the host rate, so audio-rate ring/FM sidebands stay clean
// and switching the Audio Rate mode on never clicks or changes latency.
#pragma once

#include "Params.h"
#include "RotorCore.h"
#include <atomic>

namespace rp
{
// 8th-order Butterworth low-pass as four biquads (half-band filter for the 2x oversampler).
struct HalfBand
{
    pn::Biquad s[4];
    void set (double fsHigh, double fc)
    {
        static constexpr double q[4] = { 0.50979558, 0.60134489, 0.89997622, 2.56291545 };
        for (int i = 0; i < 4; ++i) s[i].lowpass (fsHigh, fc, q[i]);
    }
    void reset() { for (auto& b : s) b.reset(); }
    inline double process (double x) { for (auto& b : s) x = b.process (x); return x; }
};

class Engine
{
public:
    void prepare (double sampleRate, int maxBlock);
    void reset();
    void setParams (const Params& p);
    const Params& params() const { return prm; }
    void process (float* left, float* right, int numSamples);

    // MIDI (call between process() calls, at the event's position)
    void sustainPedal (bool down);
    void noteOn (int note);
    void noteOff (int note);
    void allNotesOff();
    bool midiFlipped() const { return midiFlip; }

    // Lets the rotors reach their current target speed immediately (tests, offline render).
    void snapRotors() { core.snapRates(); }

    // UI telemetry (written on the audio thread, read on the message thread)
    float hornAngle() const { return tHorn.load (std::memory_order_relaxed); }
    float drumAngle() const { return tDrum.load (std::memory_order_relaxed); }
    float hornSpeed() const { return tHornHz.load (std::memory_order_relaxed); }
    float drumSpeed() const { return tDrumHz.load (std::memory_order_relaxed); }
    float limiterGain() const { return tLim.load (std::memory_order_relaxed); }
    float outputPeak() const { return tPeak.load (std::memory_order_relaxed); }
    bool  effectiveFast() const;

    double sampleRate() const { return fs; }

private:
    void applySettings();

    Params prm;
    pn::RotorCore core;
    double fs = 48000;
    HalfBand up, downL, downR;
    pn::Smoother sMix, sOut, sLimTh;
    double limEnv = 1.0, limRel = 0.999;
    bool midiFlip = false, pedalDown = false;
    int heldNotes = 0, lastNote = -1;
    bool held[128] {};

    std::atomic<float> tHorn { 0 }, tDrum { 0.3f }, tHornHz { 0 }, tDrumHz { 0 }, tLim { 1 }, tPeak { 0 };
};
} // namespace rp
