// Rotary Prism parameters: one plain struct for the engine, plus a descriptor table that the
// JUCE wrapper turns into host parameters (ids, ranges and defaults live in one place).
#pragma once

#include <cstddef>

namespace rp
{
struct Params
{
    // ---- speed ----
    int   speed = 0;              // 0 Slow, 1 Fast, 2 Brake
    int   midiSpeed = 2;          // 0 Off, 1 Pedal Hold, 2 Pedal Toggle, 3 Notes Hold

    // ---- horn ----
    float hornSlow = 0.83f;       // Hz 0.3..2
    float hornFast = 6.75f;       // Hz 3..10
    float hornAccel = 0.45f;      // s 0.05..3 (time constant)
    float hornDecel = 0.75f;      // s 0.05..5
    float hornLevel = 0.0f;       // dB -24..+6
    bool  hornAudio = false;      // audio-rate horn (ring mod / FM)
    float hornAudioHz = 440.0f;   // Hz 20..2000
    bool  noteTrack = false;      // audio rate follows the last MIDI note
    float ringFm = 0.5f;          // 0 ring .. 0.5 AM .. 1 FM

    // ---- drum ----
    float drumSlow = 0.67f;       // Hz 0.2..2
    float drumFast = 5.9f;        // Hz 2..8
    float drumAccel = 2.4f;       // s 0.3..10
    float drumDecel = 3.6f;       // s 0.3..12
    float drumLevel = 0.0f;       // dB -24..+6

    // ---- mics ----
    float micDistance = 0.25f;    // 0 close .. 1 room
    float micAngle = 150.0f;      // degrees 0..180 between the two mics
    float micBalance = 0.0f;      // -1 drum .. +1 horn

    // ---- space ----
    int   outMode = 0;            // 0 Stereo Mics, 1 Binaural (headphones), 2 Mono
    float cabSize = 0.4f;         // 0..1
    float room = 0.2f;            // 0..1

    // ---- pre ----
    float drive = 0.2f;           // 0..1
    float crossover = 800.0f;     // Hz 500..1200

    // ---- global ----
    float mix = 1.0f;             // 0..1
    float outputDb = 0.0f;        // dB -24..+12
};

enum class Kind { Float, Choice, Toggle };

struct ParamDesc
{
    const char* id;
    const char* name;
    const char* group;
    Kind kind;
    float lo, hi;
    float centre;              // skew centre for Float (value shown at mid travel), 0 = linear
    const char* unit;          // display unit for Float: %, Hz, s, dB, deg, bal
    const char* choices;       // '|' separated, for Choice
    void (*set) (Params&, float);
    float (*get) (const Params&);
};

const ParamDesc* paramTable (int& count);
const ParamDesc* findParam (const char* id);

struct Preset
{
    const char* name;
    const char* category;
    Params p;
};

const Preset* factoryPresets (int& count);
} // namespace rp
