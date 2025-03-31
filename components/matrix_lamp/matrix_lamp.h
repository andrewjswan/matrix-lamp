#pragma once

#include "esphome.h"
#include "esphome/components/template/number/template_number.h"

namespace esphome {
namespace matrix_lamp {

static const char *const TAG = "matrix_lamp";
static const char *const MATRIX_LAMP_VERSION = "2025.2.13";

#if defined(MATRIX_LAMP_USE_DISPLAY)
class MatrixLamp_Icon;
#endif // #if defined(MATRIX_LAMP_USE_DISPLAY)

#if defined(MATRIX_LAMP_BITMAP_MODE)
#define BITMAPICON MAXICONS + 1
#endif

#if defined(USE_API)
class MatrixLamp : public Component, public api::CustomAPIDevice {
#else
class MatrixLamp : public Component {
#endif
  public:
    float get_setup_priority() const override { return esphome::setup_priority::LATE; }

    void setup() override;
    void dump_config() override;
    void on_shutdown() override;

    void set_intensity(template_::TemplateNumber *intensity);
    void set_scale(template_::TemplateNumber *scale);
    void set_speed(template_::TemplateNumber *speed);

    // Reset the current effect, for example when changing the lamp state. 
    void ResetCurrentEffect();

#ifndef ORIENTATION
    // Set Matrix Orientation [1..8] instead of real matrix ORIENTATION [0..7]
    bool SetMatrixOrientation(uint8_t orientation);
#endif // #ifndef ORIENTATION

#ifndef MATRIX_TYPE
    // Set Matrix Type [1..2] instead of real matrix MATRIX_TYPE [0..1]
    bool SetMatrixType(uint8_t type);
#endif // #ifndef MATRIX_TYPE

    // Set scale for effect
    void SetIntensityForEffect(uint8_t mode, uint8_t intensity);
    // Set scale for effect
    void SetScaleForEffect(uint8_t mode, uint8_t scale);
    // Set speed for effect
    void SetSpeedForEffect(uint8_t mode, uint8_t speed);
    // Set scale from Color for effect
    void SetScaleFromColorForEffect(uint8_t mode, Color color);
    // Set custom effect
    void SetCustomEffect(uint8_t mode);

#if defined(USE_API) && defined(MATRIX_LAMP_SETTINGS)
    // Set intensity for current effect
    void set_effect_intensity(int value);
    // Set speed for current effect
    void set_effect_speed(int value);
    // Set scale for current effect
    void set_effect_scale(int value);
    // Reset brightness, speed, scale to default for current effect
    void reset_effect_settings();
#endif // #if defined(USE_API) && defined(MATRIX_LAMP_SETTINGS)

#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    bool GetRandomSettings();
    void SetRandomSettings(bool b=false);
#endif // #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    void ShowFrame(uint8_t CurrentMode, esphome::Color current_color, light::AddressableLight *p_it);

#if defined(MATRIX_LAMP_USE_DISPLAY)
    void set_display(addressable_light::AddressableLightDisplay *disp);
    void set_brightness(int value);
    void add_icon(MatrixLamp_Icon *icon);
    void show_icon(std::string icon);
    void show_icon_by_index(int icon);
    #ifdef MATRIX_LAMP_BITMAP_MODE
    void show_bitmap(std::string bitmap);
    #endif
    void hide_icon();
    void Display();
#endif // #if defined(MATRIX_LAMP_USE_DISPLAY)

  protected:
#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    bool random_settings = false;
#endif // #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

#if defined(MATRIX_LAMP_USE_DISPLAY)
    uint8_t current_icon = MAXICONS;
    uint8_t icon_count{0};
    uint8_t brightness{255};
    uint8_t target_brightness{255};
    unsigned long last_anim_time{0};
    PROGMEM MatrixLamp_Icon *icons[MAXICONS];

    addressable_light::AddressableLightDisplay *display{nullptr};
    uint8_t find_icon(std::string name);
    void update_screen();

    #ifdef MATRIX_LAMP_BITMAP_MODE
    Color* bitmap;
    #endif
#endif // #if defined(MATRIX_LAMP_USE_DISPLAY)

    esphome::template_::TemplateNumber *intensity{nullptr};
    esphome::template_::TemplateNumber *scale{nullptr};
    esphome::template_::TemplateNumber *speed{nullptr};
}; // class MatrixLamp

#if defined(MATRIX_LAMP_USE_DISPLAY)
class MatrixLamp_Icon : public animation::Animation
{
  protected:
    bool counting_up;

  public:
    MatrixLamp_Icon(const uint8_t *data_start, int width, int height, 
                    uint32_t animation_frame_count, 
                    esphome::image::ImageType type, std::string icon_name, 
                    bool revers, uint16_t frame_duration, esphome::image::Transparency transparency);
    PROGMEM std::string name;
    uint16_t frame_duration;
    void next_frame();
    bool reverse;
}; // class MatrixLamp_Icon
#endif // #if defined(MATRIX_LAMP_USE_DISPLAY)

}  // namespace matrix_lamp
}  // namespace esphome
