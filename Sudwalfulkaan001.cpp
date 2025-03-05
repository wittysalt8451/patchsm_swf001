#include "Utility/dsp.h"
#include "library/effects/ClockDetector.h"
#include "library/effects/ClockDetector.cpp"
#include "library/effects/Ducker.h"
#include "library/effects/Ducker.cpp"
#include "library/tools.cpp"
#include "daisy_patch_sm.h"
#include "library/UIManager.h"
#include "library/effects/LinkwitzRileyCrossover.h"
#include "library/effects/Saturation.h"
#include "library/effects/StereoChorus.h"
#include "library/effects/MidSide.h"
#include "library/effects/MidSide.h"
#include "library/effects/Saturation.cpp"
#include "library/effects/SwfLimiter.cpp"
#include "library/effects/LinkwitzRileyCrossover.cpp"
#include "library/effects/MidSide.cpp"
#include "library/effects/StereoChorus.cpp"
#include "library/UIManager.cpp"
#include "system.h"

#define SAMPLE_RATE 48000
#ifndef M_PI
#define M_PI 3.14159265358979323846 /* pi */
#endif

#include <cmath>
#include <algorithm>

using namespace daisy;
using namespace patch_sm;
using namespace sudwalfulkaan;

static 	DaisyPatchSM 	patch;
static 	UIManager		ui;

static 	MidSide					midside;
static 	Saturation				satL, satH;
static 	SwfLimiter				limL, limH;
static 	StereoChorus			chorusL, chorusH, chorusM, chorusS;
static	LinkwitzRileyCrossover	crossoverL, crossoverR;
static 	ClockDetector			clockDetector;
static	Ducker					ducker;

bool	bypass	= false;

int 	menu 	= UIManager::MenuState::DEFAULT;

float 	crossoverFrequency_ = 500.f;
float	monoStereoMixCV = 0.f;
float 	dryWetMixCV = 0.f;
float 	smoothedDryWet = 0.f;
float 	slewRate = 0.05f; // Adjust this for faster/slower response
float 	smoothedMonoStereoMix = 0.0f;
float 	smoothedDryWetMix = 0.0f;
float 	smoothedCrossoverCutoff = 1000.0f;
float 	smoothedSatL_Gain = 0.0f;
float 	smoothedSatH_Gain = 0.0f;
float 	smoothedMidSideWidth = 0.0f;
float 	smoothedLimL_Envelope = 0.0f;
float 	smoothedLimL_Threshold = 0.0f;
float 	smoothedLimH_Envelope = 0.0f;
float 	smoothedLimH_Threshold = 0.0f;
float 	smoothedChorusM = 0.0f;
float 	smoothedChorusH = 0.0f;
float 	smoothedChorusL = 0.0f;
float 	smoothedChorusS = 0.0f;
float 	bpm = 120.f;

void UpdateMenu() {
	// Control LED
	patch.WriteCvOut(CV_OUT_2, ui.UpdateLED());

	// Read CV inputs
	float rawMonoStereoMix = patch.GetAdcValue(patch_sm::CV_7);
	float rawDryWetMix = patch.GetAdcValue(patch_sm::CV_8);

	// Read Pot Values
	ui.SetPotValue(patch_sm::CV_1, patch.GetAdcValue(patch_sm::CV_1));
	ui.SetPotValue(patch_sm::CV_2, patch.GetAdcValue(patch_sm::CV_2));
	ui.SetPotValue(patch_sm::CV_3, patch.GetAdcValue(patch_sm::CV_3));
	ui.SetPotValue(patch_sm::CV_4, patch.GetAdcValue(patch_sm::CV_4));

	// Compute raw parameter values
	float rawCrossoverCutoff = logMapping(ui.GetPotValue(patch_sm::CV_1, UIManager::DEFAULT), 50.f, 2000.f);

	float rawSatL_Gain = ui.GetPotValue(patch_sm::CV_3, UIManager::MENU_SATURATION) + patch.GetAdcValue(patch_sm::CV_5);
	float rawSatH_Gain = ui.GetPotValue(patch_sm::CV_4, UIManager::MENU_SATURATION) + patch.GetAdcValue(patch_sm::CV_5);

	float rawMidSideWidth = ui.GetPotValue(patch_sm::CV_2, UIManager::MENU_STEREO) + ui.GetPotValue(patch_sm::CV_4, UIManager::MENU_STEREO) + rawMonoStereoMix * 2;

	float rawLimL_Envelope = ui.GetPotValue(patch_sm::CV_1, UIManager::MENU_LIMITER);
	float rawLimL_Threshold = ui.GetPotValue(patch_sm::CV_3, UIManager::MENU_LIMITER);
	float rawLimH_Envelope = ui.GetPotValue(patch_sm::CV_2, UIManager::MENU_LIMITER);
	float rawLimH_Threshold = ui.GetPotValue(patch_sm::CV_4, UIManager::MENU_LIMITER);

	float rawChorusM = ui.GetPotValue(patch_sm::CV_1, UIManager::MENU_STEREO) + patch.GetAdcValue(patch_sm::CV_6);
	float rawChorusH = ui.GetPotValue(patch_sm::CV_3, UIManager::MENU_STEREO) + patch.GetAdcValue(patch_sm::CV_6);
	float rawChorusL = ui.GetPotValue(patch_sm::CV_1, UIManager::MENU_STEREO) + patch.GetAdcValue(patch_sm::CV_6);
	float rawChorusS = ui.GetPotValue(patch_sm::CV_3, UIManager::MENU_STEREO) + patch.GetAdcValue(patch_sm::CV_6);

	// Apply slew limiting (adjust rates for different parameters)
	float slewFast = 0.1f;  // Fast transitions (e.g., gain, chorus)
	float slewMedium = 0.05f; // Mid-speed transitions (e.g., stereo width)
	float slewSlow = 0.02f;  // Slow transitions (e.g., crossover, limiter threshold)

	smoothedMonoStereoMix = SlewLimiter(rawMonoStereoMix, smoothedMonoStereoMix, slewMedium);
	smoothedDryWetMix = SlewLimiter(rawDryWetMix, smoothedDryWetMix, slewMedium);

	smoothedCrossoverCutoff = SlewLimiter(rawCrossoverCutoff, smoothedCrossoverCutoff, slewSlow);

	smoothedSatL_Gain = SlewLimiter(rawSatL_Gain, smoothedSatL_Gain, slewFast);
	smoothedSatH_Gain = SlewLimiter(rawSatH_Gain, smoothedSatH_Gain, slewFast);

	smoothedMidSideWidth = SlewLimiter(rawMidSideWidth, smoothedMidSideWidth, slewMedium);

	smoothedLimL_Envelope = SlewLimiter(rawLimL_Envelope, smoothedLimL_Envelope, slewSlow);
	smoothedLimL_Threshold = SlewLimiter(rawLimL_Threshold, smoothedLimL_Threshold, slewSlow);
	smoothedLimH_Envelope = SlewLimiter(rawLimH_Envelope, smoothedLimH_Envelope, slewSlow);
	smoothedLimH_Threshold = SlewLimiter(rawLimH_Threshold, smoothedLimH_Threshold, slewSlow);

	smoothedChorusM = SlewLimiter(rawChorusM, smoothedChorusM, slewFast);
	smoothedChorusH = SlewLimiter(rawChorusH, smoothedChorusH, slewFast);
	smoothedChorusL = SlewLimiter(rawChorusL, smoothedChorusL, slewFast);
	smoothedChorusS = SlewLimiter(rawChorusS, smoothedChorusS, slewFast);

	// Apply smoothed values to the parameters
	crossoverL.SetCutoff(smoothedCrossoverCutoff);
	crossoverR.SetCutoff(smoothedCrossoverCutoff);

	satL.SetMode(ui.GetPotValue(patch_sm::CV_1, UIManager::MENU_SATURATION));
	satL.SetGain(smoothedSatL_Gain);
	satH.SetMode(ui.GetPotValue(patch_sm::CV_2, UIManager::MENU_SATURATION));
	satH.SetGain(smoothedSatH_Gain);

	limL.SetEnvelope(smoothedLimL_Envelope);
	limL.SetThreshold(smoothedLimL_Threshold);
	limH.SetEnvelope(smoothedLimH_Envelope);
	limH.SetThreshold(smoothedLimH_Threshold);

	midside.SetWidth(smoothedMidSideWidth);

	chorusM.SetIntensity(smoothedChorusM);
	chorusH.SetIntensity(smoothedChorusH);
	chorusL.SetIntensity(smoothedChorusL);
	chorusS.SetIntensity(smoothedChorusS);

	// Dry wet controls and final gain
	float dryWet = fclamp(ui.GetPotValue(patch_sm::CV_2, UIManager::DEFAULT) + dryWetMixCV, 0.f, 1.f);

	// Apply slew limiter (adjust rate for smoothness)
	smoothedDryWet = SlewLimiter(dryWet, smoothedDryWet, slewRate);

	// Check if we need to override anything
	clockDetector.Process(patch.gate_in_1.State(), patch.system.GetNow());
	if (patch.gate_in_1.State()) {
		// Pass through the gate signal
		dsy_gpio_toggle(&patch.gate_out_1);
	}

	// 120 bpm is the initial value so all other values mean that there's a
	// gate input on the clock
	bpm = clockDetector.GetBPM();
	if (bpm != 120.f) {
		chorusL.SetRate(bpm / 60.0f);
		chorusM.SetRate(bpm / 60.0f);
		chorusH.SetRate(bpm / 60.0f);
		chorusS.SetRate(bpm / 60.0f);
		limL.SetRelease(CalculateReleaseTime(bpm));
		limH.SetRelease(CalculateReleaseTime(bpm));
		ducker.SetRelease(CalculateReleaseTime(bpm));
	}

	// Logs
	if (ui.IsBypassed() != bypass) {
		patch.PrintLine("DEBUG: Bypass = %d", static_cast<bool>(ui.IsBypassed()));

		patch.PrintLine("DEBUG: Saturation Mode L = %.2f", ui.GetPotValue(patch_sm::CV_1,UIManager::MENU_SATURATION));
		patch.PrintLine("DEBUG: Saturation Gain L = %.2f", ui.GetPotValue(patch_sm::CV_3,UIManager::MENU_SATURATION));
		patch.PrintLine("DEBUG: Saturation Mode H = %.2f", ui.GetPotValue(patch_sm::CV_2, UIManager::MENU_SATURATION));
		patch.PrintLine("DEBUG: Saturation Gain H = %.2f", ui.GetPotValue(patch_sm::CV_4,UIManager::MENU_SATURATION));

		patch.PrintLine("DEBUG: Limiter Envelope L = %.2f", ui.GetPotValue(patch_sm::CV_1, UIManager::MENU_LIMITER));
		patch.PrintLine("DEBUG: Limiter Threshold L = %.2f", ui.GetPotValue(patch_sm::CV_3, UIManager::MENU_LIMITER));
		patch.PrintLine("DEBUG: Limiter Envelope H = %.2f", ui.GetPotValue(patch_sm::CV_2, UIManager::MENU_LIMITER));
		patch.PrintLine("DEBUG: Limiter Threshold H = %.2f", ui.GetPotValue(patch_sm::CV_4, UIManager::MENU_LIMITER));

		patch.PrintLine("DEBUG: Stereo Chorus L = %.2f", ui.GetPotValue(patch_sm::CV_1, UIManager::MENU_STEREO));
		patch.PrintLine("DEBUG: Mid Gain or Side L = %.2f", ui.GetPotValue(patch_sm::CV_3, UIManager::MENU_STEREO));
		patch.PrintLine("DEBUG: Stereo Chorus H = %.2f", ui.GetPotValue(patch_sm::CV_2, UIManager::MENU_STEREO));
		patch.PrintLine("DEBUG: Stereo Gain or Side H = %.2f", ui.GetPotValue(patch_sm::CV_4, UIManager::MENU_STEREO));

		patch.PrintLine("DEBUG: Frequency = %.2f", logMapping(ui.GetPotValue(patch_sm::CV_1, UIManager::DEFAULT), 50.f, 2000.f));
		patch.PrintLine("DEBUG: Mix = %.2f", ui.GetPotValue(patch_sm::CV_2, UIManager::DEFAULT));
		patch.PrintLine("DEBUG: Overall Mid Gain or Side L = %.2f", ui.GetPotValue(patch_sm::CV_3, UIManager::DEFAULT));
		patch.PrintLine("DEBUG: Overall Stereo Gain or Side H = %.2f", ui.GetPotValue(patch_sm::CV_4, UIManager::DEFAULT));

		bypass = ui.IsBypassed();
	}

	if (ui.GetMenuState() != menu) {
		patch.PrintLine("DEBUG: MenuState = %d", static_cast<int>(ui.GetMenuState()));
		menu = ui.GetMenuState();
	}
}

void ProcessAllDualBandEffects(float& lowL, float& lowR, float& highL, float& highR) {
	// Set parameters for low
	lowL = satL.Process(lowL);
	lowR = satL.Process(lowR);
	lowL = limL.Process(lowL);
	lowR = limL.Process(lowR);
	lowL = chorusL.ProcessLeft(lowL);
	lowR = chorusL.ProcessRight(lowR);

	midside.Process(lowL, lowR);

	// Set parameters for high
	highL = satH.Process(highL);
	highR = satH.Process(highR);
	highL = limH.Process(highL);
	highR = limH.Process(highR);
	highL = chorusH.ProcessLeft(highL);
	highR = chorusH.ProcessRight(highR);

	midside.Process(highL, highR);
}

void ProcessAllMidSideEffects(float& mid, float& side) {
	// Set parameters for mid
	mid = limL.Process(mid);
	mid = satL.Process(mid);
	mid = chorusM.ProcessLeft(mid);

	// Attenuate Mid
	mid  = mid * ui.GetPotValue(patch_sm::CV_2, UIManager::DEFAULT) * (1.0f - 0.5f * monoStereoMixCV);

	// Set parameters for side
	side = satH.Process(side);
	side = limH.Process(side);
	side = chorusS.ProcessRight(side);

	// Attenuate Side
	side = side * ui.GetPotValue(patch_sm::CV_4, UIManager::DEFAULT) * monoStereoMixCV;
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	patch.ProcessAllControls();
    ducker.Update(patch.gate_in_2.State());

	for (size_t i = 0; i < size; i++)
	{
		float inL = in[0][i];
		float inR = in[1][i];
		float outL, outR = 0.f;
		float lowL, lowR, highL, highR = 0.f;

		if (ui.IsBypassed()) {
			// Mid Side processing
			float mid = (inL + inR) * 0.5f;
			float side = (inL - inR) * 0.5f;

			ProcessAllMidSideEffects(mid, side);

			outL = mid + side;
			outR = mid - side;
		} else {
			// Dual band processing
			crossoverL.Process(inL, lowL, highL);
			crossoverR.Process(inR, lowR, highR);

			ProcessAllDualBandEffects(lowL, lowR, highL, highR);

			outL = lowL * ui.GetPotValue(patch_sm::CV_3, UIManager::DEFAULT) + highL * ui.GetPotValue(patch_sm::CV_4, UIManager::DEFAULT);
			outR = lowR * ui.GetPotValue(patch_sm::CV_3, UIManager::DEFAULT) + highR * ui.GetPotValue(patch_sm::CV_4, UIManager::DEFAULT);
		}

		ducker.Process(outL, outR);

		// Apply Dry/Wet mix to output
		out[0][i] = outL * smoothedDryWet + inL * (1.f - smoothedDryWet);
		out[1][i] = outR * smoothedDryWet + inR * (1.f - smoothedDryWet);

		// Write CV out
		patch.WriteCvOut(CV_OUT_1, envelopeFollower(outL, outR) * 5.f);
	}

}

int main(void)
{
	patch.Init();

    patch.StartLog();
    patch.PrintLine("Súdwâlfulkaan birth...");

	patch.SetAudioBlockSize(4); // number of samples handled per callback
	patch.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
	patch.StartAdc();
	patch.StartAudio(AudioCallback);

	crossoverL.Init(crossoverFrequency_, SAMPLE_RATE);
	crossoverR.Init(crossoverFrequency_, SAMPLE_RATE);

	satL.Init();
	satH.Init();

	limL.Init();
	limL.SetAttack(.5f);
	limL.SetRelease(.1f);

	limH.Init();
	limH.SetAttack(.5f);
	limH.SetRelease(.1f);

	chorusL.Init(SAMPLE_RATE);
	chorusH.Init(SAMPLE_RATE);
	chorusM.Init(SAMPLE_RATE);
	chorusS.Init(SAMPLE_RATE);

	midside.Init(1.f, SAMPLE_RATE);

	ducker.Init(1.f, CalculateReleaseTime(120), SAMPLE_RATE);

	clockDetector.Init(SAMPLE_RATE, 0.1f);

	// Inform UI Manager about default values and meanwhile explain the menu settings
	ui.Init();
	ui.SetPotValue(patch_sm::CV_1, 0.0f, UIManager::MENU_SATURATION);			// Channel 1: Saturation Mode [arctan_saturation | hard_clip | sine_saturation | rectifier_saturation]
	ui.SetPotValue(patch_sm::CV_2, 0.0f, UIManager::MENU_SATURATION);			// Channel 2: Saturation Mode [arctan_saturation | hard_clip | sine_saturation | rectifier_saturation]
	ui.SetPotValue(patch_sm::CV_3, 0.05f, UIManager::MENU_SATURATION);			// Channel 1: Saturation Gain [1-20]
	ui.SetPotValue(patch_sm::CV_4, 0.05f, UIManager::MENU_SATURATION);			// Channel 2: Saturation Gain [1-20]

	ui.SetPotValue(patch_sm::CV_1, 0.0f , UIManager::MENU_LIMITER);				// Channel 1: Limiter Envelope [From slow attack/short release --> short attack/long release]
	ui.SetPotValue(patch_sm::CV_2, 0.0f , UIManager::MENU_LIMITER);				// Channel 2: Limiter Envelope [From slow attack/short release --> short attack/long release]
	ui.SetPotValue(patch_sm::CV_3, 1.0f , UIManager::MENU_LIMITER);				// Channel 1: Limiter Threshold [0-1]
	ui.SetPotValue(patch_sm::CV_4, 1.0f , UIManager::MENU_LIMITER);				// Channel 2: Limiter Threshold [0-1]

	ui.SetPotValue(patch_sm::CV_1, 0.0f , UIManager::MENU_STEREO);				// Channel 1: Chorus Intensity [dry -> wet]
	ui.SetPotValue(patch_sm::CV_2, 0.0f , UIManager::MENU_STEREO);				// Channel 2: Chorus Intensity [dry -> wet]
	ui.SetPotValue(patch_sm::CV_3, 1.0f , UIManager::MENU_STEREO);				// Channel 2: Mono/Stereo mix in Dual Band mode [0 - 2] | Mid Gain in M/S mode [0-1]
	ui.SetPotValue(patch_sm::CV_4, 1.0f , UIManager::MENU_STEREO);				// Channel 2: Mono/Stereo mix in Dual Band mode [0 - 2] | Side Gain in M/S mode [0-1]

	ui.SetPotValue(patch_sm::CV_1, 150.0f , UIManager::DEFAULT);				// Frequency [Dual band mode: Crossover frequency | M/S mode: Below Mono Frequency ]
	ui.SetPotValue(patch_sm::CV_2, 0.0f , UIManager::DEFAULT);					// Overall dry/wet mix
	ui.SetPotValue(patch_sm::CV_3, 1.0f , UIManager::DEFAULT);					// Overall volume channel 1 [Dual band mode: Low end | M/S mode: Mid]
	ui.SetPotValue(patch_sm::CV_4, 1.0f , UIManager::DEFAULT);					// Overall volume channel 2 [Dual band mode: High end | M/S mode: Side]

	while(1) {
		ui.Update();
		UpdateMenu();
		System::Delay(10);
	}
}
