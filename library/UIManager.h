#pragma once
#include "daisy_patch_sm.h"

using namespace daisy;
using namespace patch_sm;

namespace sudwalfulkaan {

    struct MenuSettings
    {
        float pot1, pot2, pot3, pot4;
    };

    class UIManager
    {
    public:
        void Init();
        void Update();
        int GetMenuState();
        float UpdateLED();
        bool IsBypassed();
        float GetPotValue(int pot_index, int menu = -1);
        void SetPotValue(int pot_index, float value, int menu = -1);
        enum MenuState { DEFAULT, MENU_SATURATION, MENU_LIMITER, MENU_STEREO };
        static constexpr int NUM_MENUS = 4;

    private:
        Switch menu_switch, bypass_switch;
        MenuState menu_state;
        MenuSettings menu_data[NUM_MENUS];
        int update_rate_;
        bool bypass_;
        float pot_values[NUM_MENUS][4];  // Store 4 pot values for each menu
        void CycleMenu();
        void SetBypass(bool bypass);
        float GetDeltaTime();
    };
}