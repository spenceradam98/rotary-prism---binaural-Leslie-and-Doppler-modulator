// Engine tests: plain C++, no framework. Run by ctest.
#include "Engine.h"
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

using namespace rp;
using pn::kPi;

static int failures = 0;
#define CHECK(cond, ...) do { if (! (cond)) { ++failures; std::printf ("FAIL %s:%d: ", __FILE__, __LINE__); std::printf (__VA_ARGS__); std::printf ("\n"); } } while (0)

struct Stats { double peak = 0, rms = 0; bool finite = true; };

static Stats run (Engine& e, std::vector<float>& l, std::vector<float>& r, int block = 256)
{
    for (size_t i = 0; i < l.size(); i += (size_t) block)
    {
        const int n = (int) std::min ((size_t) block, l.size() - i);
        e.process (l.data() + i, r.data() + i, n);
    }
    Stats s; double acc = 0;
    for (size_t i = 0; i < l.size(); ++i)
    {
        if (! std::isfinite (l[i]) || ! std::isfinite (r[i])) s.finite = false;
        s.peak = std::max (s.peak, (double) std::max (std::abs (l[i]), std::abs (r[i])));
        acc += 0.5 * (l[i] * l[i] + r[i] * r[i]);
    }
    s.rms = std::sqrt (acc / std::max<size_t> (1, l.size()));
    return s;
}

static std::vector<float> sine (double fs, double hz, double secs, double amp)
{
    std::vector<float> v ((size_t) (fs * secs));
    for (size_t i = 0; i < v.size(); ++i) v[i] = (float) (amp * std::sin (2.0 * kPi * hz * (double) i / fs));
    return v;
}

static std::vector<float> organish (double fs, double secs)
{
    // drawbar-ish chord: a few sine partials of three notes
    std::vector<float> v ((size_t) (fs * secs));
    const double notes[] = { 261.6, 329.6, 392.0 };
    for (size_t i = 0; i < v.size(); ++i)
    {
        const double t = (double) i / fs; double s = 0;
        for (double f : notes) s += std::sin (2 * kPi * f * t) + 0.5 * std::sin (4 * kPi * f * t) + 0.3 * std::sin (6 * kPi * f * t);
        v[i] = (float) (0.12 * s);
    }
    return v;
}

static double rmsOf (const std::vector<float>& v, size_t from = 0)
{
    double a = 0; for (size_t i = from; i < v.size(); ++i) a += v[i] * v[i];
    return std::sqrt (a / std::max<size_t> (1, v.size() - from));
}

// magnitude of one frequency (Goertzel-style correlation)
static double toneLevel (const std::vector<float>& v, double fs, double hz, size_t from)
{
    double re = 0, im = 0; const size_t n = v.size() - from;
    for (size_t i = from; i < v.size(); ++i)
    {
        const double ph = 2 * kPi * hz * (double) (i - from) / fs;
        re += v[i] * std::cos (ph); im += v[i] * std::sin (ph);
    }
    return 2.0 * std::sqrt (re * re + im * im) / (double) n;
}

static std::unique_ptr<Engine> makeEngine (const Params& p, double fs = 48000)
{
    auto e = std::make_unique<Engine>();
    e->setParams (p);
    e->prepare (fs, 512);
    return e;
}

int main()
{
    const double fs = 48000;

    // ---- parameter table sanity ----
    {
        int n = 0; auto* t = paramTable (n);
        CHECK (n >= 25, "expected the full parameter set, got %d", n);
        const Params def;
        for (int i = 0; i < n; ++i)
        {
            const float v = t[i].get (def);
            CHECK (v >= t[i].lo - 1e-4f && v <= t[i].hi + 1e-4f, "default of %s out of range", t[i].id);
            for (int j = i + 1; j < n; ++j) CHECK (std::strcmp (t[i].id, t[j].id) != 0, "duplicate id %s", t[i].id);
            Params p; t[i].set (p, t[i].hi);
            CHECK (std::abs (t[i].get (p) - t[i].hi) < 1e-4f, "set/get mismatch for %s", t[i].id);
        }
        CHECK (findParam ("hornAudioHz") != nullptr, "findParam");
    }

    // ---- every preset: finite, sane level, params in range ----
    {
        int np = 0; auto* ps = factoryPresets (np);
        CHECK (np >= 20, "expected at least 20 presets, got %d", np);
        int nt = 0; auto* t = paramTable (nt);
        const auto in = organish (fs, 3.0);
        const double inRms = rmsOf (in);
        for (int i = 0; i < np; ++i)
        {
            for (int k = 0; k < nt; ++k)
            {
                const float v = t[k].get (ps[i].p);
                CHECK (v >= t[k].lo - 1e-4f && v <= t[k].hi + 1e-4f, "preset %s: %s out of range", ps[i].name, t[k].id);
            }
            auto e = makeEngine (ps[i].p);
            e->noteOn (57); // so "Played Ring Mod" has a note
            auto l = in, r = in;
            const auto s = run (*e, l, r);
            CHECK (s.finite, "preset %s produced NaN/inf", ps[i].name);
            CHECK (s.peak < 2.0, "preset %s peak %.2f", ps[i].name, s.peak);
            const double ratio = s.rms / inRms;
            CHECK (ratio > 0.3 && ratio < 2.5, "preset %s level ratio %.2f", ps[i].name, ratio);
            std::printf ("  preset %-20s level %+5.1f dB  peak %.2f\n", ps[i].name, 20 * std::log10 (ratio), s.peak);
        }
    }

    // ---- silence in, silence out ----
    {
        auto e = makeEngine (Params());
        std::vector<float> l (48000, 0.0f), r (48000, 0.0f);
        const auto s = run (*e, l, r);
        CHECK (s.peak < 1e-9, "silence produced %.3g", s.peak);
    }

    // ---- mix 0 is a clean dry pass-through ----
    {
        Params p; p.mix = 0.0f;
        auto e = makeEngine (p);
        auto in = sine (fs, 440, 0.5, 0.5);
        auto l = in, r = in;
        run (*e, l, r);
        double err = 0; for (size_t i = 0; i < l.size(); ++i) err = std::max (err, (double) std::abs (l[i] - in[i]));
        CHECK (err < 1e-5, "mix 0 not dry: err %.3g", err);
    }

    // ---- rotor speeds: targets, horn faster than drum, brake stops ----
    {
        Params p; p.speed = 1;
        auto e = makeEngine (p);
        std::vector<float> l (4800, 0.0f), r (4800, 0.0f);
        double hornAt05 = 0, drumAt05 = 0;
        for (int k = 0; k < 100; ++k)   // 10 s
        {
            run (*e, l, r);
            if (k == 4) { hornAt05 = e->hornSpeed(); drumAt05 = e->drumSpeed(); }
        }
        CHECK (std::abs (e->hornSpeed() - 6.75) < 0.05, "horn fast speed %.3f", e->hornSpeed());
        CHECK (std::abs (e->drumSpeed() - 5.9) < 0.1, "drum fast speed %.3f", e->drumSpeed());
        const double hornFrac = (hornAt05 - 0.83) / (6.75 - 0.83), drumFrac = (drumAt05 - 0.67) / (5.9 - 0.67);
        CHECK (hornFrac > 0.6 && drumFrac < 0.3, "inertia: horn %.2f drum %.2f of the way after 0.5 s", hornFrac, drumFrac);

        p.speed = 2; e->setParams (p);
        for (int k = 0; k < 300; ++k) run (*e, l, r);   // 30 s
        CHECK (e->hornSpeed() < 0.01 && e->drumSpeed() < 0.05, "brake: horn %.3f drum %.3f", e->hornSpeed(), e->drumSpeed());
    }

    // ---- MIDI speed toggle ----
    {
        Params p; p.midiSpeed = 2;
        Engine e; e.setParams (p); e.prepare (fs, 512);
        CHECK (! e.effectiveFast(), "starts slow");
        e.sustainPedal (true); e.sustainPedal (false);
        CHECK (e.effectiveFast(), "pedal toggle -> fast");
        e.sustainPedal (true);
        CHECK (! e.effectiveFast(), "second press -> slow");
        p.midiSpeed = 1; e.setParams (p); e.sustainPedal (false);
        e.sustainPedal (true); CHECK (e.effectiveFast(), "pedal hold -> fast while held");
        e.sustainPedal (false); CHECK (! e.effectiveFast(), "pedal hold released -> slow");
        p.midiSpeed = 3; e.setParams (p);
        e.noteOn (60); e.noteOn (64); e.noteOff (60);
        CHECK (e.effectiveFast(), "notes hold: still a note down");
        e.noteOff (64); CHECK (! e.effectiveFast(), "notes hold: all released");
        p.midiSpeed = 2; p.speed = 2; e.setParams (p); e.sustainPedal (true);
        CHECK (! e.effectiveFast(), "brake overrides MIDI");
    }

    // ---- audio-rate horn: ring-mod sidebands at f +- rate, carrier suppressed ----
    {
        Params p; p.hornAudio = true; p.hornAudioHz = 300.0f; p.ringFm = 0.0f; p.drumLevel = -24.0f; p.room = 0.0f;
        p.micDistance = 0.0f; p.crossover = 500.0f; p.drive = 0.0f; p.outMode = 2;
        auto e = makeEngine (p); e->snapRotors();
        auto in = sine (fs, 2000, 1.0, 0.3);
        auto l = in, r = in;
        run (*e, l, r);
        const size_t from = 12000;
        const double carrier = toneLevel (l, fs, 2000, from), lower = toneLevel (l, fs, 1700, from), upper = toneLevel (l, fs, 2300, from);
        CHECK (lower > 4 * carrier && upper > 4 * carrier, "ring mod: carrier %.4f, sidebands %.4f / %.4f", carrier, lower, upper);
        CHECK (std::abs (e->hornSpeed() - 300.0) < 1.0, "audio rate speed %.2f", e->hornSpeed());
    }

    // ---- note tracking sets the ring pitch ----
    {
        Params p; p.hornAudio = true; p.noteTrack = true; p.midiSpeed = 0;
        auto e = makeEngine (p);
        e->noteOn (57);   // A3 = 220 Hz
        std::vector<float> l (48000, 0.0f), r (48000, 0.0f);
        run (*e, l, r);
        CHECK (std::abs (e->hornSpeed() - 220.0) < 1.0, "note-tracked rate %.2f", e->hornSpeed());
    }

    // ---- limiter keeps the loud audio-rate path under control ----
    {
        Params p; p.hornAudio = true; p.hornAudioHz = 90.0f; p.ringFm = 1.0f; p.drive = 1.0f; p.hornLevel = 6.0f; p.drumLevel = 6.0f; p.room = 1.0f;
        auto e = makeEngine (p); e->snapRotors();
        auto in = organish (fs, 2.0); for (auto& x : in) x *= 4.0f;
        auto l = in, r = in;
        const auto s = run (*e, l, r);
        CHECK (s.finite && s.peak < 1.0, "audio-rate limiter peak %.3f", s.peak);
    }

    // ---- mono mode: L == R ----
    {
        Params p; p.outMode = 2;
        auto e = makeEngine (p);
        auto in = organish (fs, 1.0);
        auto l = in, r = sine (fs, 100, 1.0, 0.2);
        run (*e, l, r);
        // wet parts are identical; dry is stereo, so check with mix fully wet (wet gain 1, dry 0)
        Params q = p; q.mix = 1.0f;
        auto e2 = makeEngine (q);
        auto l2 = in, r2 = in;
        run (*e2, l2, r2);
        double d = 0; for (size_t i = 0; i < l2.size(); ++i) d = std::max (d, (double) std::abs (l2[i] - r2[i]));
        CHECK (d < 1e-6, "mono mode L/R differ by %.3g", d);
    }

    // ---- stereo mics decorrelate; binaural gives a moving interaural delay ----
    {
        Params p; p.outMode = 0; p.room = 0.0f;
        auto e = makeEngine (p);
        auto in = organish (fs, 2.0);
        auto l = in, r = in;
        run (*e, l, r);
        double d = 0; for (size_t i = 24000; i < l.size(); ++i) d += std::abs (l[i] - r[i]);
        CHECK (d / 72000.0 > 1e-3, "stereo mics are not stereo (%.4g)", d / 72000.0);

        // binaural: impulse train, measure L vs R arrival over time -> lag swings both ways
        Params b; b.outMode = 1; b.room = 0.0f; b.hornSlow = 1.0f; b.drumLevel = -24.0f; b.mix = 1.0f; b.drive = 0.0f; b.micDistance = 0.0f;
        auto eb = makeEngine (b); eb->snapRotors();
        std::vector<float> il (96000, 0.0f), ir;
        for (size_t i = 0; i < il.size(); i += 480) il[i] = 0.8f;
        ir = il;
        run (*eb, il, ir);
        int earlierL = 0, earlierR = 0;
        for (size_t i = 0; i + 480 < il.size(); i += 480)
        {
            // find the peak sample index in each channel within this period
            size_t pl = i, pr = i;
            for (size_t j = i; j < i + 480; ++j) { if (std::abs (il[j]) > std::abs (il[pl])) pl = j; if (std::abs (ir[j]) > std::abs (ir[pr])) pr = j; }
            if (pl + 8 < pr) ++earlierL;
            if (pr + 8 < pl) ++earlierR;
        }
        CHECK (earlierL > 10 && earlierR > 10, "binaural ITD does not circle: L first %d, R first %d", earlierL, earlierR);
    }

    // ---- block size independence ----
    {
        Params p; p.speed = 1; p.room = 0.5f;
        auto a = makeEngine (p), b = makeEngine (p);
        auto in = organish (fs, 1.0);
        auto l1 = in, r1 = in, l2 = in, r2 = in;
        run (*a, l1, r1, 1);
        run (*b, l2, r2, 512);
        double d = 0; for (size_t i = 0; i < l1.size(); ++i) d = std::max (d, (double) std::abs (l1[i] - l2[i]));
        CHECK (d < 1e-6, "block size changes the output by %.3g", d);
    }

    // ---- other sample rates ----
    for (double rate : { 44100.0, 96000.0, 192000.0 })
    {
        Params p; p.hornAudio = true; p.hornAudioHz = 1500.0f;
        auto e = makeEngine (p, rate);
        auto in = sine (rate, 330, 1.0, 0.5);
        auto l = in, r = in;
        const auto s = run (*e, l, r);
        CHECK (s.finite && s.peak < 1.5 && s.rms > 0.05, "fs %.0f: peak %.2f rms %.3f", rate, s.peak, s.rms);
    }

    // ---- Penumbra-compatible wrapper ----
    {
        pn::RotaryClassic rc;
        rc.prepare (48000);
        rc.setParams (true, 0.2, 1.0, 0.8);
        auto in = organish (48000, 1.0);
        bool finite = true; double acc = 0;
        for (auto x : in) { double l = x, r = x; rc.process (l, r); finite &= std::isfinite (l) && std::isfinite (r); acc += l * l; }
        const double rms = std::sqrt (acc / in.size());
        CHECK (finite && rms > 0.3 * rmsOf (in) && rms < 2.5 * rmsOf (in), "RotaryClassic rms %.3f", rms);
    }

    if (failures == 0) std::printf ("All engine tests passed.\n");
    else std::printf ("%d failure(s).\n", failures);
    return failures == 0 ? 0 : 1;
}
