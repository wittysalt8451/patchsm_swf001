#ifndef DUCKER_H
#define DUCKER_H

namespace sudwalfulkaan
{
    class Ducker
    {
    public:
        /**
         * @brief Initializes the stereo ducker.
         * @param duck_amount How much to reduce volume when gate is active (0.0 - 1.0).
         * @param release_time Time in seconds to restore volume when gate is released.
         * @param sample_rate Audio sample rate (needed for smoothing calculation).
         */
        void Init(float duck_amount = 0.5f, float release_time = 0.2f, float sample_rate = 48000.0f);

        /**
         * @brief Updates the ducking state based on the gate input.
         * @param gate_state Boolean gate signal (true = ducking active).
         */
        void Update(bool gate_state);

        /**
         * @brief Applies the ducking effect to stereo samples.
         * @param left_sample The input sample for the left channel.
         * @param right_sample The input sample for the right channel.
         */
        void Process(float &left_sample, float &right_sample);

    private:
        float ducking_amount;    // Amount of volume reduction (0.0 = full duck, 1.0 = no ducking)
        float ducking_release;   // Release time in seconds
        float sample_rate;       // Audio sample rate
        float current_duck_level; // Tracks the current gain level
    };
}

#endif // DUCKER_H
