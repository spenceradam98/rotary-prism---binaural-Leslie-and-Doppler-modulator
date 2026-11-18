// Factory presets. Each starts from the defaults (a classic close-miked cabinet on Slow) and
// changes only what it needs.
#include "Params.h"

namespace rp
{
namespace
{
template <typename Fn>
Preset make (const char* name, const char* category, Fn fn)
{
    Preset pr { name, category, Params() };
    fn (pr.p);
    return pr;
}

const Preset kPresets[] = {
    make ("Classic Chorale", "Classic", [] (Params&) {}),
    make ("Classic Tremolo", "Classic", [] (Params& p) { p.speed = 1; }),
    make ("Slow Swirl", "Classic", [] (Params& p) { p.hornSlow = 0.55f; p.drumSlow = 0.42f; p.micAngle = 180.0f; p.room = 0.35f; p.micDistance = 0.4f; }),
    make ("Fast Tremolo", "Classic", [] (Params& p) { p.speed = 1; p.hornFast = 7.8f; p.drumFast = 6.6f; p.micDistance = 0.1f; p.room = 0.1f; }),
    make ("Organ Ballad", "Organ", [] (Params& p) { p.drive = 0.12f; p.room = 0.4f; p.micDistance = 0.45f; p.hornAccel = 0.8f; p.drumAccel = 3.5f; p.drumDecel = 5.0f; }),
    make ("Gospel Growl", "Organ", [] (Params& p) { p.drive = 0.7f; p.speed = 1; p.hornLevel = 1.5f; p.crossover = 900.0f; p.micDistance = 0.15f; }),
    make ("Jazz Club Cab", "Organ", [] (Params& p) { p.drive = 0.08f; p.cabSize = 0.6f; p.room = 0.3f; p.micDistance = 0.35f; p.micAngle = 120.0f; }),
    make ("Guitar Through Cab", "Guitar", [] (Params& p) { p.drive = 0.35f; p.crossover = 1000.0f; p.hornLevel = -1.5f; p.micDistance = 0.2f; p.cabSize = 0.3f; p.mix = 0.85f; }),
    make ("Brake Stop", "Motion", [] (Params& p) { p.speed = 2; p.hornDecel = 1.6f; p.drumDecel = 4.0f; p.room = 0.25f; }),
    make ("Lazy Ramp", "Motion", [] (Params& p) { p.hornAccel = 2.2f; p.hornDecel = 3.5f; p.drumAccel = 7.0f; p.drumDecel = 10.0f; p.midiSpeed = 1; }),
    make ("Snap Ramp", "Motion", [] (Params& p) { p.hornAccel = 0.08f; p.hornDecel = 0.12f; p.drumAccel = 0.4f; p.drumDecel = 0.6f; }),
    make ("Head Circle", "Binaural", [] (Params& p) { p.outMode = 1; p.hornSlow = 0.35f; p.drumSlow = 0.25f; p.micDistance = 0.1f; p.room = 0.15f; }),
    make ("Head Spin Fast", "Binaural", [] (Params& p) { p.outMode = 1; p.speed = 1; p.hornFast = 4.5f; p.drumFast = 3.2f; p.room = 0.1f; }),
    make ("Orbiting Choir", "Binaural", [] (Params& p) { p.outMode = 1; p.hornSlow = 0.3f; p.drumSlow = 0.22f; p.room = 0.55f; p.micDistance = 0.5f; p.mix = 0.9f; }),
    make ("Ring Horn 440", "Audio Rate", [] (Params& p) { p.hornAudio = true; p.hornAudioHz = 440.0f; p.ringFm = 0.0f; p.drumLevel = -6.0f; p.room = 0.1f; p.mix = 0.8f; }),
    make ("Metallic FM", "Audio Rate", [] (Params& p) { p.hornAudio = true; p.hornAudioHz = 155.6f; p.ringFm = 1.0f; p.crossover = 600.0f; p.room = 0.2f; }),
    make ("Bell Tones", "Audio Rate", [] (Params& p) { p.hornAudio = true; p.hornAudioHz = 523.3f; p.ringFm = 0.25f; p.cabSize = 0.7f; p.room = 0.45f; p.mix = 0.7f; }),
    make ("Played Ring Mod", "Audio Rate", [] (Params& p) { p.hornAudio = true; p.noteTrack = true; p.ringFm = 0.1f; p.midiSpeed = 0; p.mix = 0.75f; }),
    make ("Wobble Vox", "Vocal", [] (Params& p) { p.hornSlow = 1.6f; p.drumSlow = 1.1f; p.crossover = 650.0f; p.drive = 0.05f; p.mix = 0.6f; p.room = 0.2f; }),
    make ("Underwater Leslie", "FX", [] (Params& p) { p.speed = 1; p.hornFast = 9.5f; p.cabSize = 1.0f; p.crossover = 1200.0f; p.hornLevel = -4.0f; p.room = 0.6f; p.micDistance = 0.7f; }),
    make ("Far Room Swirl", "FX", [] (Params& p) { p.micDistance = 0.9f; p.room = 0.8f; p.micAngle = 90.0f; p.mix = 0.8f; }),
    make ("Mono Cabinet", "Classic", [] (Params& p) { p.outMode = 2; p.micDistance = 0.15f; p.room = 0.05f; }),
};
} // namespace

const Preset* factoryPresets (int& count)
{
    count = (int) (sizeof (kPresets) / sizeof (kPresets[0]));
    return kPresets;
}
} // namespace rp
