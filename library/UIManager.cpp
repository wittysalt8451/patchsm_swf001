#include "UIManager.h"
#include "daisy_patch_sm.h"
#include <math.h>
#include "system.h"

using namespace daisy;
using namespace patch_sm;
using namespace sudwalfulkaan;

float last_pot_values[4] = {0.0f, 0.0f, 0.0f, 0.0f};

// LED variables
float brightness = 0.0f;
float time_elapsed = 0.0f;
bool led_on = false;
float eruption_timer = 0.0f;
uint32_t last_time = 0;

void UIManager::Init()
{
    menu_state = DEFAULT;
    menu_switch.Init(DaisyPatchSM::B7, 0, Switch::TYPE_MOMENTARY, Switch::POLARITY_INVERTED, Switch::PULL_UP);
    bypass_switch.Init(DaisyPatchSM::B8, 0, Switch::TYPE_TOGGLE, Switch::POLARITY_NORMAL, Switch::PULL_UP);
}

void UIManager::Update()
{
    menu_switch.Debounce();
    bypass_switch.Debounce();

    if (menu_switch.FallingEdge()) {
        CycleMenu();
    }

    if (bypass_switch.Pressed()) {
        SetBypass(true);
    } else {
        SetBypass(false);
    }

    // Update LED feedback based on the current menu state
    UpdateLED();
}

void UIManager::SetPotValue(int pot_index, float value, int menu)
{
    // Check which menu to update
    int index = (menu == -1) ? static_cast<int>(menu_state) : menu;

    if (index >= 0 && index < NUM_MENUS && pot_index >= 0 && pot_index < 4)
    {
        const float threshold = 0.01f; // Sensitivity threshold

        // Check if the pot was actually turned if trying to set other than current menu
        if (menu >= 0 || fabsf(value - last_pot_values[pot_index]) > threshold)
        {
            last_pot_values[pot_index] = value; // Update last raw pot value
            (&menu_data[index].pot1)[pot_index] = value; // Store in menu
        }
    }
}

float UIManager::GetPotValue(int pot_index, int menu)
{
    int index = (menu == -1) ? static_cast<int>(menu_state) : menu;
    if (index >= 0 && index < NUM_MENUS && pot_index >= 0 && pot_index < 4)
    {
        return (&menu_data[index].pot1)[pot_index];
    }
    return 0.0f;  // Return default if out of range
}


void UIManager::CycleMenu()
{
    // Cycle through your menu states
    menu_state = static_cast<MenuState>((menu_state + 1) % NUM_MENUS);

    // Reset menu LED timer
    time_elapsed = 0.0f;
}

void UIManager::SetBypass(bool bypass) {
    bypass_ = bypass;
}

bool UIManager::IsBypassed() {
    return bypass_;
}

int UIManager::GetMenuState() {
    return menu_state;
}

float UIManager::GetDeltaTime() {
    uint32_t current_time = System::GetNow(); // Get current time in ms
    float delta_time = (current_time - last_time) / 1000.0f; // Convert to seconds
    last_time = current_time; // Update last time

    return delta_time > 0 ? delta_time : 0; // Ensure no negative values
}

float UIManager::UpdateLED() {
    float delta = GetDeltaTime();
    time_elapsed += delta;

    if (static_cast<int>(menu_state) == DEFAULT) {
        // Live Menu - Volcano Effect
        eruption_timer += delta;

        if (eruption_timer > 5.0) { // Simulate eruption every ~5s
            brightness = 5.0; // Bright flash
            eruption_timer = 0.0;
        } else {
            // Flickering glow effect (lava flow)
            brightness = 1.5 + 1.5 * sin(time_elapsed * 2.0) + (rand() % 10) * 0.1;
        }
    } else {
        // Settings Menus - Blinking
        int blinks = static_cast<int>(menu_state); // 1 blink for menu 1, 2 blinks for menu 2, etc.
        float blink_duration = 0.2; // Each blink is 200ms ON, 200ms OFF
        float total_blink_time = blinks * blink_duration * 2; // Time spent blinking
        float cycle_time = total_blink_time + 2.0; // Total cycle = blinking + 2s pause

        if (time_elapsed < total_blink_time) {
            // Blinking phase
            int blink_index = (int)(time_elapsed / blink_duration); // Count blink steps
            led_on = (blink_index % 2 == 0); // ON for even steps, OFF for odd
            brightness = led_on ? 5.0 : 0.0;
        } else {
            // Pause phase (LED stays off)
            brightness = 0.0;
        }

        if (time_elapsed > cycle_time) {
            time_elapsed = 0.0; // Reset cycle
        }
    }

    // Write brightness to LED
    return brightness;
}
