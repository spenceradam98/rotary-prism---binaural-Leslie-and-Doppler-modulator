#include "Params.h"
#include <cstring>

namespace rp
{
#define F(field) [] (Params& p, float v) { p.field = v; }, [] (const Params& p) { return (float) p.field; }
#define I(field) [] (Params& p, float v) { p.field = (int) (v + 0.5f); }, [] (const Params& p) { return (float) p.field; }
#define B(field) [] (Params& p, float v) { p.field = v > 0.5f; }, [] (const Params& p) { return p.field ? 1.0f : 0.0f; }

static const ParamDesc kTable[] = {
    { "speed",       "Speed",          "Speed", Kind::Choice, 0, 2, 0, "", "Slow|Fast|Brake", I (speed) },
    { "midiSpeed",   "MIDI Speed",     "Speed", Kind::Choice, 0, 3, 0, "", "Off|Pedal Hold|Pedal Toggle|Notes Hold", I (midiSpeed) },

    { "hornSlow",    "Horn Slow",      "Horn",  Kind::Float, 0.3f, 2.0f, 0.83f, "Hz", nullptr, F (hornSlow) },
    { "hornFast",    "Horn Fast",      "Horn",  Kind::Float, 3.0f, 10.0f, 0, "Hz", nullptr, F (hornFast) },
    { "hornAccel",   "Horn Accel",     "Horn",  Kind::Float, 0.05f, 3.0f, 0.6f, "s", nullptr, F (hornAccel) },
    { "hornDecel",   "Horn Decel",     "Horn",  Kind::Float, 0.05f, 5.0f, 0.9f, "s", nullptr, F (hornDecel) },
    { "hornLevel",   "Horn Level",     "Horn",  Kind::Float, -24.0f, 6.0f, 0, "dB", nullptr, F (hornLevel) },
    { "hornAudio",   "Audio Rate",     "Horn",  Kind::Toggle, 0, 1, 0, "", nullptr, B (hornAudio) },
    { "hornAudioHz", "Audio Rate Hz",  "Horn",  Kind::Float, 20.0f, 2000.0f, 200.0f, "Hz", nullptr, F (hornAudioHz) },
    { "noteTrack",   "Rate From Note", "Horn",  Kind::Toggle, 0, 1, 0, "", nullptr, B (noteTrack) },
    { "ringFm",      "Ring/FM",        "Horn",  Kind::Float, 0.0f, 1.0f, 0, "%", nullptr, F (ringFm) },

    { "drumSlow",    "Drum Slow",      "Drum",  Kind::Float, 0.2f, 2.0f, 0.67f, "Hz", nullptr, F (drumSlow) },
    { "drumFast",    "Drum Fast",      "Drum",  Kind::Float, 2.0f, 8.0f, 0, "Hz", nullptr, F (drumFast) },
    { "drumAccel",   "Drum Accel",     "Drum",  Kind::Float, 0.3f, 10.0f, 2.5f, "s", nullptr, F (drumAccel) },
    { "drumDecel",   "Drum Decel",     "Drum",  Kind::Float, 0.3f, 12.0f, 3.5f, "s", nullptr, F (drumDecel) },
    { "drumLevel",   "Drum Level",     "Drum",  Kind::Float, -24.0f, 6.0f, 0, "dB", nullptr, F (drumLevel) },

    { "micDistance", "Mic Distance",   "Mics",  Kind::Float, 0.0f, 1.0f, 0, "%", nullptr, F (micDistance) },
    { "micAngle",    "Mic Angle",      "Mics",  Kind::Float, 0.0f, 180.0f, 0, "deg", nullptr, F (micAngle) },
    { "micBalance",  "Horn/Drum Mic",  "Mics",  Kind::Float, -1.0f, 1.0f, 0, "bal", nullptr, F (micBalance) },

    { "outMode",     "Output Mode",    "Space", Kind::Choice, 0, 2, 0, "", "Stereo Mics|Binaural (Headphones)|Mono", I (outMode) },
    { "cabSize",     "Cabinet Size",   "Space", Kind::Float, 0.0f, 1.0f, 0, "%", nullptr, F (cabSize) },
    { "room",        "Room",           "Space", Kind::Float, 0.0f, 1.0f, 0, "%", nullptr, F (room) },

    { "drive",       "Drive",          "Pre",   Kind::Float, 0.0f, 1.0f, 0, "%", nullptr, F (drive) },
    { "crossover",   "Crossover",      "Pre",   Kind::Float, 500.0f, 1200.0f, 0, "Hz", nullptr, F (crossover) },

    { "mix",         "Mix",            "Global", Kind::Float, 0.0f, 1.0f, 0, "%", nullptr, F (mix) },
    { "output",      "Output",         "Global", Kind::Float, -24.0f, 12.0f, 0, "dB", nullptr, F (outputDb) },
};

#undef F
#undef I
#undef B

const ParamDesc* paramTable (int& count)
{
    count = (int) (sizeof (kTable) / sizeof (kTable[0]));
    return kTable;
}

const ParamDesc* findParam (const char* id)
{
    int n = 0; auto* t = paramTable (n);
    for (int i = 0; i < n; ++i)
        if (std::strcmp (t[i].id, id) == 0) return t + i;
    return nullptr;
}
} // namespace rp
