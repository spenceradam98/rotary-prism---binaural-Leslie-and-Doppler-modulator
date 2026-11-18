# Rotary Prism: design (v0.1)

*Working name. Product name, company and plugin codes sit in `CMakeLists.txt`; the UI title in
`Source/plugin/PluginEditor.cpp`.*

## Goal

The deep version of Penumbra's Rotary stage: a psychedelic stereo rotary speaker for organs, pads,
guitars and vocals. Independent horn and drum rotors, movable mics, a binaural mode for
headphones, and a horn that can spin far past real speeds into audio-rate ring mod and FM.
Pairs with emerald-haze and paisley-synth (organs and pads) and with guitar after velvet-fuzz.

## Signal flow

```
in (stereo) -> mono sum -> 2x upsample
   -> valve preamp (Drive) -> LR4 crossover (500..1200 Hz)
        -> horn rotor (treble)  \
        -> drum rotor (bass)     > pickups: Stereo Mics / Binaural / Mono  + room reflections + short FDN
   -> 2x downsample -> wet limiter -> Mix with dry -> Output
```

Zero latency. The rotor model always runs at twice the host rate, so audio-rate sidebands stay
clean and switching Audio Rate on or off never changes latency or clicks. All DSP is scalar C++17
in `Source/engine` (no JUCE, no SIMD intrinsics, no `-march` flags).

### The shared rotor model (`RotorCore.h`)

Grown out of Penumbra's `Rotary.h` (same preamp, crossover, default speeds and inertia, Doppler +
directivity). The plan was to move that code into a shared engine file and extend it, not to
write a second rotary model. Each repo builds on its own, so the file is copied and must stay
byte-identical in both. It also contains `RotaryClassic`, which keeps Penumbra's original
`Rotary` API (`setParams (fast, drive, mix, spread)`), so Penumbra can switch to the new file
without other changes. There's a test for that wrapper here.

Each rotor is a moving source read from its own delay line:

* **Doppler:** the read position swings with the rotor angle as seen from each pickup. Cabinet
  Size sets the rotor radius, so the horn swing runs from 0.2 to 0.6 ms (Penumbra's is 0.35 ms).
* **Directivity:** amplitude modulation, and the horn gets darker when it faces away (a one-pole
  shade filter, as in Penumbra).
* **Mic distance:** close mics get deep AM. Moving them out flattens the AM, lowers the direct
  sound and raises the room.
* **Room:** four wall reflections, each a pickup at its own angle and delay (4 to 17 ms), feeding
  a 4-line damped FDN whose decay grows with Room.
* **Cabinet:** a drum body resonance (165 Hz small, 85 Hz large) and a mild horn "honk" at 1.7 kHz.

### Output modes

| Mode | What it does |
|---|---|
| Stereo Mics | Two mics at ±Angle/2 around the cabinet (0 to 180°). Each mic sees its own Doppler and AM phase. |
| Binaural (Headphones) | The listener's head is inside the rotation. Per ear: interaural delay up to 0.66 ms (ITD) and a head-shadow low-pass and level drop on the far ear (ILD), plus slight darkening behind the head. The sound circles the head instead of just panning. Only works on headphones, which is why the mode's name says so. |
| Mono | One mic in front, same signal on both sides (the room folds to centre too). |

### Audio-rate horn

With **Audio Rate** on, the horn's target speed becomes the Rate knob (20 Hz to 2 kHz), or the
last MIDI note when **Rate From Note** is on, so the ring mod plays in tune. Between 12 and 30 Hz
the model crossfades from speaker behaviour to modulator behaviour:

* the directivity becomes ring and AM modulation: **Ring/FM** at 0 gives bipolar ring mod with the
  carrier suppressed, at 50% classic AM, and at 100% no AM,
* the Doppler becomes FM, with a depth that scales as 1/rate so that it stays musical at any rate.

Above 12 Hz the speed glides in pitch (30 ms), so notes slide like portamento. Turning Audio Rate
off lets the horn spin back down through every speed with its Decel time. A wet-path peak limiter
(instant attack, 120 ms release) holds the audio-rate path at -1 dBFS. When Audio Rate is off it
only acts as a safety above +6 dBFS.

### Speed and MIDI

Slow / Fast / Brake switch. Brake coasts both rotors to a stop with their own Decel times. MIDI
input is optional (**MIDI Speed**):

| Setting | Behaviour |
|---|---|
| Off | MIDI is ignored for speed (notes still set the ring pitch when Rate From Note is on). |
| Pedal Hold | Sustain pedal (CC 64) flips Slow/Fast while it's held. |
| Pedal Toggle | Each press of the pedal flips Slow/Fast (default). |
| Notes Hold | Any held note flips Slow/Fast. |

MIDI events are handled at their exact sample position. Brake always wins.

## Controls

| Section | Controls |
|---|---|
| Speed | Slow / Fast / Brake, MIDI Speed |
| Horn | Slow, Fast, Accel, Decel, Level, Audio Rate + Rate (20 Hz to 2 kHz), Rate From Note, Ring/FM |
| Drum | Slow, Fast, Accel, Decel, Level |
| Mics | Distance, Angle, Horn/Drum balance (also by dragging the mics in the cabinet view) |
| Space | Output mode, Cabinet size, Room |
| Pre | Drive (valve preamp), Crossover (500 to 1200 Hz) |
| Output | Mix, Output |

Accel and Decel are the rotor time constants. Defaults match Penumbra: horn 0.45 s up and 0.75 s
down, drum 2.4 s up and 3.6 s down, which gives the familiar staggered ramp.

## UI

A top-down cutaway of the cabinet. The drum (with its scoop opening) and the double horn spin at
the real modelled speed and slow down with the brake. Light trails show the speed, and above about
10 Hz the horn blurs into a shimmering ring. The two mic icons can be dragged: in and out sets
Distance, around the cabinet sets Angle. In Binaural mode a listener's head replaces the mics,
facing up, with L and R marked. Readouts show the live horn and drum speeds and limiter gain
reduction.

Six looks: Penumbra's five (Spectrum, Console, Eclipse, Moon Dust, Tape Room) plus **Kaleido**
(the default), a prism theme with rainbow light trails, rainbow knob arcs and faint prism rays.
The theme and window size (resizable, fixed aspect, 60 to 200%) are saved with the session.

## Presets (22)

Classic: Classic Chorale, Classic Tremolo, Slow Swirl, Fast Tremolo, Mono Cabinet ·
Organ: Organ Ballad, Gospel Growl, Jazz Club Cab · Guitar: Guitar Through Cab ·
Motion: Brake Stop, Lazy Ramp, Snap Ramp · Binaural: Head Circle, Head Spin Fast, Orbiting Choir ·
Audio Rate: Ring Horn 440, Metallic FM, Bell Tones, Played Ring Mod · Vocal: Wobble Vox ·
FX: Underwater Leslie, Far Room Swirl.

All presets sit within about -8 to +1 dB of the input level on an organ chord (the tests print the
table).

## Tests (`tests/EngineTests.cpp`)

Parameter table (ranges, unique ids, set/get), every preset (finite, peak and level window),
silence in gives silence out, Mix 0 is a clean dry pass-through, rotor speeds and inertia (horn
reaches speed well before the drum), Brake stops both rotors, the MIDI speed modes, ring-mod
sidebands at f ± rate with the carrier suppressed, note tracking, the limiter on a deliberately
hot audio-rate patch, Mono mode L = R, stereo mics decorrelate, binaural ITD swings both ways,
block-size independence, 44.1/96/192 kHz, and the Penumbra-compatible wrapper.

## Not done yet / ideas

* Penumbra still has its own `Rotary.h`. Switching it to `RotorCore.h` + `RotaryClassic` is a
  small follow-up in that repo.
* No preset demo render yet (Penumbra has a renderer tool that could be copied).
