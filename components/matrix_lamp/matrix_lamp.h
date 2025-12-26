#pragma once

#include "esphome/components/template/number/template_number.h"

namespace esphome {
namespace matrix_lamp {

static const char *const TAG = "matrix_lamp";
static const char *const MATRIX_LAMP_VERSION = "2025.10.1";

#if defined(MATRIX_LAMP_TRIGGERS)
class MatrixLampEffectStartTrigger;
#endif

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
    void reset_current_effect();

#ifndef ORIENTATION
    // Set Matrix Orientation [1..8] instead of real matrix ORIENTATION [0..7]
    bool set_matrix_orientation(uint8_t orientation);
#endif // #ifndef ORIENTATION

#ifndef MATRIX_TYPE
    // Set Matrix Type [1..2] instead of real matrix MATRIX_TYPE [0..1]
    bool set_matrix_type(uint8_t type);
#endif // #ifndef MATRIX_TYPE

    // Set / Get intensity for effect
    void set_intensity_for_effect(uint8_t mode, uint8_t intensity);
    uint8_t get_intensity_for_effect(uint8_t mode);

    // Set / Get scale for effect
    void set_scale_for_effect(uint8_t mode, uint8_t scale);
    uint8_t get_scale_for_effect(uint8_t mode);

    // Set / Get speed for effect
    void set_speed_for_effect(uint8_t mode, uint8_t speed);
    uint8_t get_speed_for_effect(uint8_t mode);

    // Set scale from Color for effect
    void set_scale_from_color_for_effect(uint8_t mode, Color color);

    // Set custom effect mode
    void set_custom_effect_mode(uint8_t mode);

    // Set / Get intensity for current effect
    void set_effect_intensity(int32_t value);
    uint8_t get_effect_intensity();

    // Set / Get speed for current effect
    void set_effect_speed(int32_t value);
    uint8_t get_effect_speed();

    // Set / Get scale for current effect
    void set_effect_scale(int32_t value);
    uint8_t get_effect_scale();

    // Reset brightness, speed, scale to default for current effect
    void reset_effect_settings();

#if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    // Set / Get random settings
    bool get_random_settings();
    void set_random_settings(bool b=false);
#endif // #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)

    void ShowFrame(uint8_t CurrentMode, esphome::Color current_color, light::AddressableLight *p_it);

#if defined(MATRIX_LAMP_USE_DISPLAY)
    void set_display(addressable_light::AddressableLightDisplay *disp);
    void set_brightness(int32_t value);
    void add_icon(MatrixLamp_Icon *icon);
    void show_icon(std::string icon);
    void show_icon_by_index(int32_t icon);
    #ifdef MATRIX_LAMP_BITMAP_MODE
    void show_bitmap(std::string bitmap);
    #endif
    void hide_icon();
    void Display();
#endif // #if defined(MATRIX_LAMP_USE_DISPLAY)

#if defined(MATRIX_LAMP_TRIGGERS)
    void add_on_effect_start_trigger(MatrixLampEffectStartTrigger *t) { this->on_effect_start_triggers_.push_back(t); }
#endif

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

#if defined(MATRIX_LAMP_TRIGGERS)
    std::vector<MatrixLampEffectStartTrigger *> on_effect_start_triggers_;
#endif
}; // class MatrixLamp

#if defined(MATRIX_LAMP_USE_DISPLAY)
class MatrixLamp_Icon : public animation::Animation
{
  protected:
    bool counting_up;

  public:
    MatrixLamp_Icon(const uint8_t *data_start, uint32_t width, uint32_t height, 
                    uint32_t animation_frame_count, 
                    esphome::image::ImageType type, std::string icon_name, 
                    bool revers, uint16_t frame_duration, esphome::image::Transparency transparency);
    PROGMEM std::string name;
    uint16_t frame_duration;
    void next_frame();
    bool reverse;
}; // class MatrixLamp_Icon
#endif // #if defined(MATRIX_LAMP_USE_DISPLAY)

#if defined(MATRIX_LAMP_TRIGGERS)
class MatrixLampEffectStartTrigger : public Trigger<uint8_t, uint8_t, uint8_t, uint8_t>
{
  public:
    explicit MatrixLampEffectStartTrigger(MatrixLamp *parent) { parent->add_on_effect_start_trigger(this); }
    void process(uint8_t, uint8_t, uint8_t, uint8_t);
}; // class 
#endif

}  // namespace matrix_lamp
}  // namespace esphome
