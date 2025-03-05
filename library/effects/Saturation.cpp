#include "Saturation.h"
#include <cmath>
#include "daisysp.h"

using namespace sudwalfulkaan;
using namespace daisysp;

void Saturation::Init(float gain = 1.0f, int mode = 0, float drywet = 1.f) {
    gain_ = gain;
    mode_ = mode;
    drywet_ = drywet;
}
void Saturation::setDryWet(float drywet) {
    drywet_ = drywet;
}
void Saturation::SetGain(float gain) {
    gain_ = gain * 9.f + 1.f;
}

void Saturation::SetMode(float mode) {
    // Ensure mode is within the expected range
    mode = std::max(0.0f, std::min(0.99f, mode));

    // Map mode to an evenly distributed integer value
    mode_ = static_cast<int>(mode * (Saturation::NUM_SATURATIONS));
}

int Saturation::GetMode() {
    return mode_;
}

float Saturation::Process(float in) {
    float x = in * gain_;
    float saturated = ApplySaturation(x);
    return drywet_ * saturated  + ((1.f - drywet_) * in);
}

float Saturation::ApplySaturation(float x) {
    switch (mode_) {
        case 0: return arctan_saturation(x);            // A bit more hooky, but still gentle (YES)
        case 1: return sine_saturation(x);              // Nice crusty edge (YES)
        case 2: return rectifier_saturation(x);         // Creats more highend and low oomph (OK)
        case 3: return bitcrush_saturation(x, 2); // Typical Rectifier sound (YES)
        default: return 0;
    }
}

// Saturation Functions

// Soft with bigger bass
float Saturation::soft_clipping(float x) { return x / (1 + std::fabs(x)); }
float Saturation::tanh_saturation(float x) { return std::tanh(x); }
 // A bit more hooky, but still gentle
float Saturation::arctan_saturation(float x) { return std::atan(x); }
float Saturation::cubic_saturation(float x) { return x - (1.0f / 3.0f) * x * x * x; }
// Same as sine but bit more natural
float Saturation::quadratic_saturation(float x) { return (x > 1.0f) ? (2.0f - (1.0f / x)) : x; }
// Creats more highend and low oomph
float Saturation::sine_saturation(float x) { return std::sin(x); }
// Extremez
float Saturation::exp_saturation(float x) { return 1 - exp(-std::fabs(x)) * (x < 0 ? -1 : 1); }
 // Nice crusty edge
float Saturation::hard_clip(float x) { return (std::max(-1.0f, std::min(1.0f, x)))*.8f; }
float Saturation::bitcrush_saturation(float x, int bits) {
    float scale = std::pow(2, bits);
    return std::round(x * scale) / scale;
}
 // Typical Rectifier sound
float Saturation::rectifier_saturation(float x) { return std::fabs(x); }
