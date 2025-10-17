## Home Assistant services

There are services and parameters to use.

You can call this from, e.g., the developer tools service.

[![Open your Home Assistant instance with the developer tools service.](https://my.home-assistant.io/badges/developer_services.svg)](https://my.home-assistant.io/redirect/developer_services/)

!!! note "Services"
    ``` { .copy .annotate }
      brightness {"value"}
      show_icon {"icon_name"}
      show_bitmap {"bitmap"}
      hide_icon

      set_effect_intensity {"value"}
      set_effect_speed {"value"}
      set_effect_scale {"value"}
      reset_effect_settings
    ```

## Lambda

You can use the above functions also in [lambdas](https://esphome.io/automations/actions#lambda-action) in your esphome YAML.

!!! note "Lambda functions"
    ``` { .cpp .copy .annotate }
      void set_brightness(int value);
      void show_icon(std::string icon);
      void show_icon_by_index(int icon);
      void show_bitmap(std::string bitmap);
      void hide_icon();

      // Reset the current effect, for example when changing the lamp state. 
      void reset_current_effect();

    #ifndef ORIENTATION
      // Set Matrix Orientation [1..8] instead of real matrix ORIENTATION [0..7]
      bool set_matrix_orientation(uint8_t orientation);
    #endif

    #ifndef MATRIX_TYPE
      // Set Matrix Type [1..2] instead of real matrix MATRIX_TYPE [0..1]
      bool set_matrix_type(uint8_t type);
    #endif

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
      void set_effect_intensity(int value);
      uint8_t get_effect_intensity();

      // Set / Get speed for current effect
      void set_effect_speed(int value);
      uint8_t get_effect_speed();

      // Set / Get scale for current effect
      void set_effect_scale(int value);
      uint8_t get_effect_scale();

      // Reset brightness, speed, scale to default for current effect
      void reset_effect_settings();

    #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
      // Set / Get random settings
      bool get_random_settings();
      void set_random_settings(bool b=false);
    #endif // #if defined(RANDOM_SETTINGS_IN_CYCLE_MODE)
    ```
