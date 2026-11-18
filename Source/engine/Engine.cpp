#include "Engine.h"

namespace rp
{
using pn::dbToGain;

void Engine::prepare (double sampleRate, int)
{
    fs = sampleRate;
    applySettings();
    core.prepare (2.0 * fs);
    const double fc = std::min (0.45 * fs, 20000.0);
    up.set (2.0 * fs, fc); downL.set (2.0 * fs, fc); downR.set (2.0 * fs, fc);
    sMix.set (fs, 20.0); sOut.set (fs, 20.0); sLimTh.set (fs, 40.0);
    limRel = std::exp (-1.0 / (0.12 * fs));
    reset();
}

void Engine::reset()
{
    core.reset();
    up.reset(); downL.reset(); downR.reset();
    sMix.reset (prm.mix); sOut.reset (dbToGain (prm.outputDb)); sLimTh.reset (prm.hornAudio ? 0.89 : 2.0);
    limEnv = 1.0;
}

void Engine::setParams (const Params& p)
{
    prm = p;
    if (prm.midiSpeed == 0) midiFlip = false;
    applySettings();
}

bool Engine::effectiveFast() const
{
    if (prm.speed == 2) return false;
    return (prm.speed == 1) != midiFlip;
}

void Engine::applySettings()
{
    pn::RotorSettings s;
    s.speed = prm.speed == 2 ? pn::RotorSettings::Brake : (effectiveFast() ? pn::RotorSettings::Fast : pn::RotorSettings::Slow);
    s.hornSlow = prm.hornSlow; s.hornFast = prm.hornFast; s.hornAccel = prm.hornAccel; s.hornDecel = prm.hornDecel;
    s.drumSlow = prm.drumSlow; s.drumFast = prm.drumFast; s.drumAccel = prm.drumAccel; s.drumDecel = prm.drumDecel;
    s.hornLevel = dbToGain (prm.hornLevel); s.drumLevel = dbToGain (prm.drumLevel);
    s.hornAudio = prm.hornAudio;
    s.hornAudioHz = (prm.noteTrack && lastNote >= 0) ? 440.0 * std::pow (2.0, (lastNote - 69) / 12.0) : prm.hornAudioHz;
    s.ringFm = prm.ringFm;
    s.distance = prm.micDistance; s.micAngleDeg = prm.micAngle; s.micBalance = prm.micBalance;
    s.output = prm.outMode; s.cabSize = prm.cabSize; s.room = prm.room;
    s.drive = prm.drive; s.crossover = prm.crossover;
    core.setSettings (s);
}

void Engine::sustainPedal (bool down)
{
    if (down == pedalDown) return;
    pedalDown = down;
    if (prm.midiSpeed == 1) midiFlip = down;
    else if (prm.midiSpeed == 2 && down) midiFlip = ! midiFlip;
    applySettings();
}

void Engine::noteOn (int note)
{
    if (note < 0 || note > 127) return;
    if (! held[note]) { held[note] = true; ++heldNotes; }
    lastNote = note;
    if (prm.midiSpeed == 3) midiFlip = true;
    applySettings();
}

void Engine::noteOff (int note)
{
    if (note < 0 || note > 127 || ! held[note]) return;
    held[note] = false; --heldNotes;
    if (prm.midiSpeed == 3 && heldNotes == 0) midiFlip = false;
    applySettings();
}

void Engine::allNotesOff()
{
    for (auto& h : held) h = false;
    heldNotes = 0;
    if (prm.midiSpeed == 3) midiFlip = false;
    applySettings();
}

void Engine::process (float* left, float* right, int n)
{
    const double outTarget = dbToGain (prm.outputDb);
    const double limTarget = prm.hornAudio ? 0.89 : 2.0;   // -1 dBFS on the audio-rate path, else safety only
    double peak = 0, minLim = 1.0;
    for (int i = 0; i < n; ++i)
    {
        const double dl = left[i], dr = right[i];
        const double x = 0.5 * (dl + dr);

        // 2x oversampled rotor core
        double aL, aR, bL, bR;
        core.process (up.process (2.0 * x), aL, aR);
        core.process (up.process (0.0), bL, bR);
        downL.process (aL); downR.process (aR);
        double wl = downL.process (bL), wr = downR.process (bR);

        // wet-path peak limiter: instant attack, 120 ms release, soft clip above the threshold
        const double th = sLimTh.next (limTarget);
        const double pk = std::max (std::abs (wl), std::abs (wr));
        const double want = pk > th ? th / pk : 1.0;
        limEnv = want < limEnv ? want : want + limRel * (limEnv - want);
        wl *= limEnv; wr *= limEnv;
        minLim = std::min (minLim, limEnv);

        const double m = sMix.next (prm.mix), g = sOut.next (outTarget);
        const double ol = (dl * pn::dryGain (m) + wl * pn::wetGain (m)) * g;
        const double orr = (dr * pn::dryGain (m) + wr * pn::wetGain (m)) * g;
        left[i] = (float) ol; right[i] = (float) orr;
        peak = std::max (peak, std::max (std::abs (ol), std::abs (orr)));
    }
    tHorn.store ((float) core.hornAngle(), std::memory_order_relaxed);
    tDrum.store ((float) core.drumAngle(), std::memory_order_relaxed);
    tHornHz.store ((float) core.hornSpeed(), std::memory_order_relaxed);
    tDrumHz.store ((float) core.drumSpeed(), std::memory_order_relaxed);
    tLim.store ((float) minLim, std::memory_order_relaxed);
    tPeak.store ((float) peak, std::memory_order_relaxed);
}
} // namespace rp
