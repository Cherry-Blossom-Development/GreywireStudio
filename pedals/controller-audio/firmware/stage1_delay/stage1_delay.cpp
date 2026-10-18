// Switchyard Audio, breadboard stage 1: test delay.
//
// The Daisy produces only the echoes (the wet signal). The dry guitar signal
// stays analog on the breadboard and is mixed back in by the TL072 mixer, so
// this program never outputs the input directly.
//
// Wiring: SWITCHYARD_AUDIO_BREADBOARD_STAGE1.md (in pedals/controller-audio).
//   Pin 22 (A0)  Time pot      50 ms to 1 s
//   Pin 23 (A1)  Repeats pot   one echo to almost endless
//   Pin 24 (A2)  Mix pot       echo level
//   Pin 8  (D7)  Footswitch    effect on/off (to ground, internal pull-up)
//   Pin 9  (D8)  Relay driver  high = effect in, low = bypass
//   Pin 10 (D9)  Status LED    lit when the effect is on

#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

namespace
{
constexpr float kSampleRate   = 48000.f;
constexpr float kMinDelaySec  = 0.05f;
constexpr float kMaxDelaySec  = 1.0f;
constexpr float kMaxFeedback  = 0.95f; // below 1 so the repeats always die away
constexpr float kRepeatToneHz = 5000.f; // repeats get a little darker each time

// A little longer than the longest delay, so the interpolated read never wraps.
constexpr size_t kDelaySamples = static_cast<size_t>(kSampleRate * kMaxDelaySec) + 64;

enum AdcChannel
{
    kTime,
    kRepeats,
    kMix,
    kNumAdc
};

DaisySeed hw;
Switch    footswitch;
GPIO      relay;
GPIO      led;

DelayLine<float, kDelaySamples> DSY_SDRAM_BSS delay_line;
OnePole                                       repeat_tone;

bool  effect_on      = false;
float delay_samples  = kSampleRate * 0.3f; // smoothed, follows the Time pot
float feedback       = 0.f;
float wet_level      = 0.f;

void SetEffect(bool on)
{
    effect_on = on;
    relay.Write(on);
    led.Write(on);
}

// Pots move in coarse steps and the ADC is a little noisy, so every setting
// is smoothed. Without this, turning Time would make the echoes crackle.
void ReadControls()
{
    // Time is exponential: equal turns of the knob feel like equal changes.
    float t      = hw.adc.GetFloat(kTime);
    float target = kMinDelaySec * powf(kMaxDelaySec / kMinDelaySec, t) * kSampleRate;
    fonepole(delay_samples, target, 0.0005f);

    fonepole(feedback, hw.adc.GetFloat(kRepeats) * kMaxFeedback, 0.002f);
    fonepole(wet_level, hw.adc.GetFloat(kMix), 0.002f);
}

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    footswitch.Debounce();
    if(footswitch.RisingEdge())
        SetEffect(!effect_on);

    for(size_t i = 0; i < size; i++)
    {
        ReadControls();

        float echo = delay_line.Read(delay_samples);

        // While bypassed, stop feeding new sound in so the line empties out
        // and switching back on doesn't replay old echoes.
        float input  = effect_on ? in[0][i] : 0.f;
        float repeat = repeat_tone.Process(echo) * feedback;
        delay_line.Write(SoftClip(input + repeat));

        out[0][i] = echo * wet_level;
        out[1][i] = out[0][i];
    }
}
} // namespace

int main(void)
{
    hw.Init();
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    hw.SetAudioBlockSize(48);

    relay.Init(seed::D8, GPIO::Mode::OUTPUT);
    led.Init(seed::D9, GPIO::Mode::OUTPUT);
    SetEffect(false);

    footswitch.Init(seed::D7, hw.AudioCallbackRate());

    AdcChannelConfig adc[kNumAdc];
    adc[kTime].InitSingle(seed::A0);
    adc[kRepeats].InitSingle(seed::A1);
    adc[kMix].InitSingle(seed::A2);
    hw.adc.Init(adc, kNumAdc);
    hw.adc.Start();

    delay_line.Init();
    repeat_tone.Init();
    repeat_tone.SetFrequency(kRepeatToneHz / hw.AudioSampleRate());

    hw.StartAudio(AudioCallback);

    for(;;) {}
}
