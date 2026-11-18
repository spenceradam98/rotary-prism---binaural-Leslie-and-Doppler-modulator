// Extra DSP building blocks for Penumbra (on top of Dsp.h). Plain C++17, scalar only:
// no SIMD intrinsics and no -march flags, so it runs on any x86-64 (macOS 11 Intel) or arm64.
#pragma once

#include "Dsp.h"

namespace pn
{
constexpr double kTwoPi = 2.0 * kPi;

// One-pole parameter smoother (time constant in ms).
struct Smoother
{
    double a = 0, y = 0;
    void set (double fs, double ms) { a = std::exp (-1.0 / (std::max (0.01, ms) * 0.001 * fs)); }
    void reset (double v) { y = v; }
    inline double next (double target) { y = target + a * (y - target); return y; }
};

// Phase accumulator LFO, phase in [0,1).
struct Lfo
{
    double phase = 0, inc = 0;
    void setRate (double fs, double hz) { inc = hz / fs; }
    inline double tick() { phase += inc; if (phase >= 1.0) phase -= std::floor (phase); return phase; }
    static double sine (double p) { return std::sin (kTwoPi * p); }
    static double tri (double p) { return 1.0 - 4.0 * std::abs (wrap (p) - 0.5); }
    static double wrap (double p) { return p - std::floor (p); }
};

// Numerically stable log(cosh(x)).
inline double logCosh (double x)
{
    const double a = std::abs (x);
    return a + std::log1p (std::exp (-2.0 * a)) - 0.69314718055994531;
}

// Biased tanh clipper with first-order antiderivative anti-aliasing (ADAA).
// f(x) = tanh(x + b) - tanh(b)  (asymmetric for b != 0, zero at x = 0)
// y[n] = (F(x[n]) - F(x[n-1])) / (x[n] - x[n-1]), F = antiderivative of f.
// This removes most of the aliasing of a high-gain clipper without oversampling.
struct AdaaTanh
{
    double bias = 0, offs = 0, xPrev = 0, fPrev = 0;
    void setBias (double b) { bias = b; offs = std::tanh (b); reset(); }
    void reset() { xPrev = 0; fPrev = F (0.0); }
    double F (double x) const { return logCosh (x + bias) - offs * x; }
    double f (double x) const { return std::tanh (x + bias) - offs; }
    inline double process (double x)
    {
        const double Fx = F (x), d = x - xPrev;
        const double y = std::abs (d) < 1e-6 ? f (0.5 * (x + xPrev)) : (Fx - fPrev) / d;
        xPrev = x; fPrev = Fx;
        return y;
    }
};

// First-order allpass y = a*x + z, z = x - a*y.
struct Allpass1
{
    double z = 0;
    inline double process (double x, double a) { const double y = a * x + z; z = x - a * y; return y; }
    static double coef (double fs, double fc)
    {
        const double t = std::tan (kPi * clampd (fc, 10.0, fs * 0.45) / fs);
        return (t - 1.0) / (t + 1.0);
    }
};

// Schroeder allpass with an integer delay (reverb diffusers).
struct DiffuserAP
{
    std::vector<double> buf; int w = 0, len = 1;
    void allocate (int n) { len = std::max (1, n); buf.assign ((size_t) len, 0.0); w = 0; }
    void clear() { std::fill (buf.begin(), buf.end(), 0.0); }
    inline double process (double x, double g)
    {
        const double d = buf[(size_t) w];
        const double v = x - g * d;
        buf[(size_t) w] = v;
        if (++w >= len) w = 0;
        return d + g * v;
    }
    // read the internal line, i samples after the oldest (for multi-tap outputs)
    inline double tap (int i) const { int r = w - 1 - i; while (r < 0) r += len; return buf[(size_t) r]; }
};

// Cubic (Hermite) fractional read for modulated delays.
inline double tapCubic (const DelayLine& d, double pos)
{
    const int i = (int) pos; const double f = pos - i;
    const double xm1 = d.tap (std::max (0, i - 1)), x0 = d.tap (i), x1 = d.tap (i + 1), x2 = d.tap (i + 2);
    const double c1 = 0.5 * (x1 - xm1), c2 = xm1 - 2.5 * x0 + 2.0 * x1 - 0.5 * x2, c3 = 0.5 * (x2 - xm1) + 1.5 * (x0 - x1);
    return ((c3 * f + c2) * f + c1) * f + x0;
}

// Four-pole transistor-ladder-style low-pass (zero-delay feedback, tanh at the input),
// in the spirit of the VCS3 diode-ladder filter. Self-oscillates near res = 1.
struct Ladder
{
    double s[4] {}, G = 0.5, k = 0;
    void reset() { s[0] = s[1] = s[2] = s[3] = 0; }
    void set (double fs, double fc, double res)
    {
        const double g = std::tan (kPi * clampd (fc, 16.0, fs * 0.45) / fs);
        G = g / (1.0 + g);
        k = 4.15 * clampd (res, 0.0, 1.0);
    }
    inline double process (double x)
    {
        const double b = 1.0 - G;
        const double sigma = G * G * G * b * s[0] + G * G * b * s[1] + G * b * s[2] + b * s[3];
        const double G4 = G * G * G * G;
        const double y4est = (G4 * x + sigma) / (1.0 + k * G4);
        double u = std::tanh (x * (1.0 + 0.35 * k) - k * y4est);
        for (int i = 0; i < 4; ++i)
        {
            const double v = (u - s[i]) * G;
            const double y = v + s[i];
            s[i] = y + v;
            u = y;
        }
        return u;
    }
};

// Equal-ish wet/dry law used by every module with a Mix control: dry stays at unity
// up to 50 %, then fades; wet rises to unity at 50 %.
inline double dryGain (double mix) { return std::min (1.0, 2.0 * (1.0 - mix)); }
inline double wetGain (double mix) { return std::min (1.0, 2.0 * mix); }
} // namespace pn
