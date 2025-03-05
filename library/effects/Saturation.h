#pragma once
#ifndef SATURATION_H
#define SATURATION_H

#include <cmath>
#include <algorithm>

namespace sudwalfulkaan {
    class Saturation {
    public:
        void Init(float gain, int mode, float drywet);
        void setDryWet(float drywet);
        void SetGain(float gain);
        int GetMode();
        void SetMode(float mode);
        float Process(float in);
        static constexpr int NUM_SATURATIONS = 4;

    private:
        float gain_;
        int mode_;
        float drywet_;

        float ApplySaturation(float x);

        static float soft_clipping(float x);
        static float tanh_saturation(float x);
        static float arctan_saturation(float x);
        static float cubic_saturation(float x);
        static float quadratic_saturation(float x);
        static float sine_saturation(float x);
        static float exp_saturation(float x);
        static float hard_clip(float x);
        static float bitcrush_saturation(float x, int bits = 8);
        static float rectifier_saturation(float x);
    };
}

#endif // SATURATION_H