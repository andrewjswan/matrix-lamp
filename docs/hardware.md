## Pin scheme

|         |ESP32       |ESP32C3             |ESP32S2             | ESP32S3    |
|---------|------------|--------------------|--------------------|------------|
|LED      |GPIO3       |GPIO3               |GPIO3               |GPIO3       |
|Button   |GPIO35      |GPIO10              |GPIO35              |GPIO35      |
|Pullup   |False       |True                |True                |True        |
|Platform |neopixelbus |esp32_rmt_led_strip |esp32_rmt_led_strip |neopixelbus |

## Settings override

If you need to change any settings, you can override them using `substitutions`, for example:

!!! example
    ``` { .yaml .copy .annotate }
    substitutions:
      pin_leds: "GPIO32"
      type_leds: "GRB"
      color_correct_bright: "70%"
      pin_button: "GPIO35"
      pin_pullup: "false"
    ```
