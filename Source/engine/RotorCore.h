// RotorCore: the shared rotary-speaker (Leslie-type) model.
//
// Grown out of Penumbra's Rotary stage and meant to be shared: keep this file byte-identical in
// every repo that uses it (rotary-prism now, penumbra when it picks up the deep version), so a fix
// in one can be copied to the other. It only depends on Dsp.h and Blocks.h, which are already
// identical copies of Penumbra's.
//
// Model, per input sample (mono, after the caller's oversampling if any):
//   valve preamp (biased tanh) -> Linkwitz-Riley 4th-order crossover (500..1200 Hz)
//   -> horn (treble) and drum (bass) rotors, each with its own speed, acceleration and braking.
// Each rotor is a moving source read from its own delay line:
//   * Doppler: the read position moves with the rotor angle as seen from each pickup
//   * directivity: amplitude modulation, and the horn gets darker when it faces away
//   * room: four wall reflections (each a pickup at its own angle and extra delay) + a small FDN
// Pickups depend on the output mode:
//   Stereo mics: two mics around the cabinet, spaced by the Angle control
//   Binaural:    the listener's head sits inside the rotation; per-ear delay (ITD) and head shadow
//                (ILD, a low-pass on the far ear) make the rotors circle the head. Headphones only.
//   Mono:        one mic in front, same signal on both sides
// Audio rate: when the horn spins above ~12 Hz the model crossfades from "speaker" behaviour to
// "modulator" behaviour: the directivity becomes ring/AM modulation (bipolar at Ring, unipolar in
// the middle, none at FM) and the Doppler becomes FM with a depth that scales as 1/rate so that it
// stays musical up to 2 kHz. Run it oversampled (the caller does 2x) to keep sidebands clean.
//
// RotaryClassic at the bottom wraps the core in Penumbra's original Rotary API
// (setParams (fast, drive, mix, spread)), so Penumbra can switch to this file without other changes.
#pragma once

#include "Blocks.h"

namespace pn
{
struct RotorSettings
{
    enum Speed { Slow = 0, Fast = 1, Brake = 2 };
    enum Output { StereoMics = 0, Binaural = 1, Mono = 2 };

    int speed = Slow;
    double hornSlow = 0.83, hornFast = 6.75, hornAccel = 0.45, hornDecel = 0.75;   // Hz, Hz, s, s (time constants)
    double drumSlow = 0.67, drumFast = 5.9, drumAccel = 2.4, drumDecel = 3.6;
    double hornLevel = 1.0, drumLevel = 1.0;   // linear gains
    bool   hornAudio = false;                  // spin the horn at audio rate
    double hornAudioHz = 440.0;                // 20..2000
    double ringFm = 0.5;                       // 0 ring mod .. 0.5 AM .. 1 FM (audio-rate only)
    double distance = 0.25;                    // 0 close mics .. 1 across the room
    double micAngleDeg = 150.0;                // spacing between the two mics, 0..180
    double micBalance = 0.0;                   // -1 drum mic only .. +1 horn mic only
    int    output = StereoMics;
    double cabSize = 0.4;                      // 0..1: rotor radius (Doppler depth) and body resonance
    double room = 0.2;                         // 0..1 reflections + short ambience
    double drive = 0.2;                        // 0..1 valve preamp
    double crossover = 800.0;                  // Hz
};

class RotorCore
{
public:
    static constexpr double kAudioStart = 12.0, kAudioFull = 30.0;   // horn Hz where speaker -> modulator

    void prepare (double sampleRate)
    {
        fs = sampleRate;
        const int len = (int) (0.05 * fs) + 16;
        hornDelay.allocate (len);
        drumDelay.allocate (len);
        setCrossover (set.crossover, true);
        hornShade.set (fs, 3500.0);
        earLp.set (fs, 1500.0);
        hornHonk.peak (fs, 1700.0, 1.4, 2.0);
        updateBody (true);
        sDrive.set (fs, 30.0); sHorn.set (fs, 30.0); sDrum.set (fs, 30.0); sDist.set (fs, 50.0);
        sRoom.set (fs, 50.0); sAngle.set (fs, 60.0); sBal.set (fs, 30.0); sRingFm.set (fs, 20.0); sMode.set (fs, 25.0);
        const double fdnMs[4] = { 29.7, 37.1, 41.3, 53.9 };
        for (int i = 0; i < 4; ++i) { fdn[i].allocate ((int) (fdnMs[i] * 0.001 * fs) + 4); fdnLen[i] = (int) (fdnMs[i] * 0.001 * fs); fdnDamp[i].set (fs, 5200.0); }
        hornRate = set.hornSlow; drumRate = set.drumSlow;
        updateCoefs();
        reset();
    }

    void reset()
    {
        hornDelay.clear(); drumDelay.clear();
        for (auto* b : { &lp1, &lp2, &hp1, &hp2, &drumBody, &hornHonk }) b->reset();
        for (auto& d : fdn) d.clear();
        for (auto& d : fdnDamp) d.reset();
        for (auto& s : shade) s = 0;
        for (auto& e : ear) e.reset();
        sDrive.reset (set.drive); sHorn.reset (set.hornLevel); sDrum.reset (set.drumLevel); sDist.reset (set.distance);
        sRoom.reset (set.room); sAngle.reset (set.micAngleDeg); sBal.reset (set.micBalance); sRingFm.reset (set.ringFm);
        modeMix[0] = set.output == RotorSettings::StereoMics; modeMix[1] = set.output == RotorSettings::Binaural; modeMix[2] = set.output == RotorSettings::Mono;
        for (int i = 0; i < 3; ++i) modeS[i].reset (modeMix[i]);
    }

    void setSettings (const RotorSettings& s)
    {
        const bool xo = std::abs (s.crossover - set.crossover) > 0.5, body = std::abs (s.cabSize - set.cabSize) > 1e-4;
        set = s;
        if (xo) setCrossover (set.crossover, false);
        if (body) updateBody (false);
        for (int i = 0; i < 3; ++i) modeMix[i] = set.output == i ? 1.0 : 0.0;
        updateCoefs();
    }
    const RotorSettings& settings() const { return set; }

    // Jump both rotors to their current target speed (used after loading a preset offline / in tests).
    void snapRates() { hornRate = hornTarget(); drumRate = drumTarget(); }

    inline void process (double in, double& outL, double& outR)
    {
        advanceRotors();

        const double dr = sDrive.next (set.drive);
        const double pg = 1.0 + 9.0 * dr * dr;
        double x = std::tanh (in * pg + 0.1 * dr) - std::tanh (0.1 * dr);
        x /= std::max (1.0, 0.6 * pg);

        const double hi = hornHonk.process (hp2.process (hp1.process (x)));
        const double lo = drumBody.process (lp2.process (lp1.process (x)));
        hornDelay.push (hi);
        drumDelay.push (lo);

        const double dist = sDist.next (set.distance), room = sRoom.next (set.room), bal = sBal.next (set.micBalance);
        const double fm = sRingFm.next (set.ringFm);
        const double hornLvl = sHorn.next (set.hornLevel) * (bal < 0 ? 1.0 + bal : 1.0);
        const double drumLvl = sDrum.next (set.drumLevel) * (bal > 0 ? 1.0 - bal : 1.0);
        const double mS = modeS[0].next (modeMix[0]), mB = modeS[1].next (modeMix[1]), mM = modeS[2].next (modeMix[2]);

        // audio-rate blend
        const double k = clampd ((hornRate - kAudioStart) / (kAudioFull - kAudioStart), 0.0, 1.0);
        const double exH = (1.0 - k) * hornExc + k * fm * std::min (0.004, 0.55 / (kTwoPi * std::max (1.0, hornRate)));
        const double exD = drumExc;
        const double amdH = 0.65 * (1.0 - 0.6 * dist), amdD = 0.22 * (1.0 - 0.6 * dist);
        auto hornGain = [&] (double c, double amd)
        {
            const double speaker = 1.0 - amd * (0.5 - 0.5 * c);
            const double audio = (1.0 - fm) * c + fm;
            return speaker + k * (audio - speaker);
        };

        const double ha = kTwoPi * hornPhase, da = kTwoPi * drumPhase;
        const double sH = std::sin (ha), cH = std::cos (ha), sD = std::sin (da), cD = std::cos (da);
        const double baseH = 0.0012 * fs, baseD = 0.0010 * fs;

        double dirL = 0, dirR = 0;

        // ---- stereo mics / mono: pickups at azimuth +-off/2 (or 0) ----
        if (mS + mM > 1e-6)
        {
            const double off = 0.5 * sAngle.next (set.micAngleDeg) * kPi / 180.0;
            const double so = std::sin (off), co = std::cos (off);
            double hm[3], dm[3];
            // 0 = left mic (-off), 1 = right mic (+off), 2 = front mic (mono)
            const double ss[3] = { sH * co + cH * so, sH * co - cH * so, sH }, cc[3] = { cH * co - sH * so, cH * co + sH * so, cH };
            const double sd[3] = { sD * co + cD * so, sD * co - cD * so, sD }, cd[3] = { cD * co - sD * so, cD * co + sD * so, cD };
            const int first = mS > 1e-6 ? 0 : 2, last = mM > 1e-6 ? 2 : 1;
            for (int m = first; m <= last; ++m)
            {
                const double v = tapCubic (hornDelay, baseH + exH * fs * ss[m]);
                const double facing = 0.5 + 0.5 * cc[m];
                shade[m] = v + hornShade.a * (shade[m] - v);   // duller when the horn faces away
                const double toned = facing * v + (1.0 - facing) * shade[m];
                hm[m] = (toned + k * (v - toned)) * hornGain (cc[m], amdH);
                dm[m] = tapCubic (drumDelay, baseD + exD * fs * sd[m]) * (1.0 - amdD * (0.5 - 0.5 * cd[m]));
            }
            if (mS > 1e-6)
            {
                dirL += mS * (hm[0] * hornLvl * 1.25 + dm[0] * drumLvl);
                dirR += mS * (hm[1] * hornLvl * 1.25 + dm[1] * drumLvl);
            }
            if (mM > 1e-6)
            {
                const double mono = hm[2] * hornLvl * 1.25 + dm[2] * drumLvl;
                dirL += mM * mono; dirR += mM * mono;
            }
        }
        else
            sAngle.next (set.micAngleDeg);

        // ---- binaural: rotors circle the listener's head ----
        if (mB > 1e-6)
        {
            const double ild = 0.85 * (1.0 - 0.5 * dist);
            const double itd = 0.00033 * fs;                       // half the max interaural delay
            double out[2];
            for (int e = 0; e < 2; ++e)
            {
                const double side = e == 0 ? sH : -sH;             // > 0: horn on the far side of this ear
                const double sideD = e == 0 ? sD : -sD;
                const double vh = tapCubic (hornDelay, baseH + itd * (1.0 + side) - 0.5 * exH * fs * cH);
                const double shadow = clampd (ild * std::max (0.0, side) + 0.25 * std::max (0.0, -cH), 0.0, 1.0);
                const double lp = ear[e].process (vh);
                const double h = (vh + shadow * (lp - vh)) * (1.0 - 0.35 * shadow) * hornGain (cH, 0.5 * amdH);
                const double vd = tapCubic (drumDelay, baseD + itd * (1.0 + sideD) - 0.5 * exD * fs * cD);
                const double d = vd * (1.0 - 0.2 * ild * std::max (0.0, sideD));
                out[e] = h * hornLvl * 1.25 + d * drumLvl;
            }
            dirL += mB * out[0]; dirR += mB * out[1];
        }

        // ---- room: four wall reflections + short diffuse tail ----
        double rl = 0, rr = 0;
        const double roomAmt = room * (0.35 + 0.9 * dist);
        if (roomAmt > 1e-5)
        {
            static constexpr double wallS[4] = { 0.7173560909, 0.7457052122, -0.6877661592, -0.7726222100 };   // sin of 0.8, 2.3, 3.9, 5.4 rad
            static constexpr double wallC[4] = { 0.6967067093, -0.6662760213, -0.7259323042, 0.6349294281 };
            static constexpr double wallMs[4] = { 4.1, 7.3, 11.9, 16.7 };
            for (int w = 0; w < 4; ++w)
            {
                const double s = sH * wallC[w] - cH * wallS[w], c = cH * wallC[w] + sH * wallS[w];
                const double sd = sD * wallC[w] - cD * wallS[w], cd = cD * wallC[w] + sD * wallS[w];
                const double wd = wallMs[w] * 0.001 * fs;
                const double h = hornDelay.tapFrac (baseH + wd + exH * fs * s) * hornGain (c, 0.5);
                const double d = drumDelay.tapFrac (baseD + wd + exD * fs * sd) * (0.85 + 0.15 * cd);
                const double v = (h * hornLvl * 1.25 + d * drumLvl) * (0.8 - 0.1 * w);
                (w & 1 ? rr : rl) += v;
            }
            // 4-line FDN (Householder feedback), damped
            double y[4];
            for (int i = 0; i < 4; ++i) y[i] = fdnDamp[i].process (fdn[i].tap (fdnLen[i] - 1));
            const double sum = 0.5 * (y[0] + y[1] + y[2] + y[3]);
            const double inp[4] = { rl, rr, rl, rr };
            for (int i = 0; i < 4; ++i) fdn[i].push (0.6 * inp[i] + fdnG[i] * (y[i] - sum));
            rl = rl + 0.7 * (y[0] - y[2]) + 0.35 * y[1];
            rr = rr + 0.7 * (y[1] - y[3]) + 0.35 * y[2];
        }
        else
        {
            // keep the tank draining cleanly when Room is at zero
            for (int i = 0; i < 4; ++i) fdn[i].push (0.0);
        }

        if (mM > 1e-6)   // mono: fold the room to the centre as well
        {
            const double c = 0.5 * (rl + rr);
            rl += mM * (c - rl); rr += mM * (c - rr);
        }

        const double direct = 1.0 - 0.45 * dist;
        outL = (dirL * direct + rl * roomAmt) * 0.8;
        outR = (dirR * direct + rr * roomAmt) * 0.8;
    }

    // UI / test telemetry
    double hornAngle() const { return hornPhase; }      // 0..1 turns
    double drumAngle() const { return drumPhase; }
    double hornSpeed() const { return hornRate; }       // Hz
    double drumSpeed() const { return drumRate; }
    double audioAmount() const { return clampd ((hornRate - kAudioStart) / (kAudioFull - kAudioStart), 0.0, 1.0); }

private:
    double hornTarget() const
    {
        if (set.speed == RotorSettings::Brake) return 0.0;
        if (set.hornAudio) return clampd (set.hornAudioHz, 1.0, 4000.0);
        return set.speed == RotorSettings::Fast ? set.hornFast : set.hornSlow;
    }
    double drumTarget() const
    {
        if (set.speed == RotorSettings::Brake) return 0.0;
        return set.speed == RotorSettings::Fast ? set.drumFast : set.drumSlow;
    }

    void updateCoefs()
    {
        auto c = [this] (double tau) { return 1.0 - std::exp (-1.0 / (std::max (0.005, tau) * fs)); };
        hornUp = c (set.hornAccel); hornDown = c (set.hornDecel);
        drumUp = c (set.drumAccel); drumDown = c (set.drumDecel);
        glide = c (0.03);
        const double rt = 0.35 + 0.9 * set.room;   // ambience decay, s
        for (int i = 0; i < 4; ++i) fdnG[i] = std::pow (10.0, -3.0 * fdnLen[i] / (rt * fs));
        hornExc = (0.0002 + 0.0004 * set.cabSize);    // s: horn path-length swing (Penumbra's horn = 0.35 ms)
        drumExc = (0.00006 + 0.00016 * set.cabSize); // s
    }

    void setCrossover (double hz, bool resetState)
    {
        lp1.lowpass (fs, hz, 0.7071); lp2.lowpass (fs, hz, 0.7071);
        hp1.highpass (fs, hz, 0.7071); hp2.highpass (fs, hz, 0.7071);
        if (resetState) { lp1.reset(); lp2.reset(); hp1.reset(); hp2.reset(); }
    }

    void updateBody (bool resetState)
    {
        drumBody.peak (fs, 165.0 - 80.0 * set.cabSize, 1.2, 1.0 + 3.0 * set.cabSize);
        if (resetState) drumBody.reset();
    }

    inline void advanceRotors()
    {
        const double hT = hornTarget(), dT = drumTarget();
        if (hT > kAudioStart && hornRate > kAudioStart)
            hornRate *= std::exp (glide * std::log (hT / hornRate));   // audio rate: glide in pitch
        else
            hornRate += (hT - hornRate) * (hT > hornRate ? hornUp : hornDown);
        drumRate += (dT - drumRate) * (dT > drumRate ? drumUp : drumDown);
        hornPhase += hornRate / fs; if (hornPhase >= 1.0) hornPhase -= std::floor (hornPhase);
        drumPhase += drumRate / fs; if (drumPhase >= 1.0) drumPhase -= std::floor (drumPhase);
    }

    RotorSettings set;
    double fs = 96000;
    double hornRate = 0.83, drumRate = 0.67, hornPhase = 0.0, drumPhase = 0.3;
    double hornUp = 0, hornDown = 0, drumUp = 0, drumDown = 0, glide = 0;
    double hornExc = 0.00035, drumExc = 0.00012;
    DelayLine hornDelay, drumDelay;
    Biquad lp1, lp2, hp1, hp2, drumBody, hornHonk;
    OnePoleLP hornShade, earLp;
    double shade[3] {};
    struct EarFilter { double z = 0; const OnePoleLP* p = nullptr; void reset() { z = 0; } inline double process (double x) { z = x + p->a * (z - x); return z; } };
    EarFilter ear[2] { { 0, &earLp }, { 0, &earLp } };
    DelayLine fdn[4];
    int fdnLen[4] { 1, 1, 1, 1 };
    double fdnG[4] {};
    OnePoleLP fdnDamp[4];
    Smoother sDrive, sHorn, sDrum, sDist, sRoom, sAngle, sBal, sRingFm, sMode;
    Smoother modeS[3];
    double modeMix[3] { 1, 0, 0 };
};

// Penumbra's original Rotary API on top of RotorCore (no oversampling, same speeds and inertia).
class RotaryClassic
{
public:
    void prepare (double sampleRate)
    {
        RotorSettings s;
        s.cabSize = 0.375; s.room = 0.0; s.distance = 0.0;   // 0.35 ms horn swing, dry close mics
        core.setSettings (s);
        core.prepare (sampleRate);
        sMix.set (sampleRate, 30.0);
        sMix.reset (mix);
    }
    void reset() { core.reset(); sMix.reset (mix); }
    void setParams (bool fastIn, double driveIn, double mixIn, double spreadIn)
    {
        mix = mixIn;
        RotorSettings s = core.settings();
        s.speed = fastIn ? RotorSettings::Fast : RotorSettings::Slow;
        s.drive = driveIn;
        s.micAngleDeg = 180.0 * (0.15 + 0.85 * spreadIn);
        core.setSettings (s);
    }
    inline void process (double& l, double& r)
    {
        const double m = sMix.next (mix);
        double wl, wr;
        core.process (0.5 * (l + r), wl, wr);
        wl *= 1.25; wr *= 1.25;   // RotorCore scales its output by 0.8
        l = l * dryGain (m) + wl * wetGain (m);
        r = r * dryGain (m) + wr * wetGain (m);
    }
    double getHornAngle() const { return core.hornAngle(); }
    double getDrumAngle() const { return core.drumAngle(); }

private:
    RotorCore core;
    Smoother sMix;
    double mix = 1.0;
};
} // namespace pn
