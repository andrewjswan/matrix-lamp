# ESPHome Matrix Lamp
[![matrix-lamp_badge](https://img.shields.io/static/v1?label=Matrix&message=Lamp&color=orange&logo=lighthouse&logoColor=white)](https://github.com/andrewjswan/esphome-components/)
[![esp32_arduino](https://img.shields.io/badge/ESP32-Arduino-darkcyan.svg)](https://esphome.io/)
[![Build](https://github.com/andrewjswan/matrix-lamp/actions/workflows/build.yaml/badge.svg)](https://github.com/andrewjswan/matrix-lamp/actions/workflows/build.yaml)
[![esphome_badge](https://img.shields.io/static/v1?label=ESPHome&message=Components&color=blue&logo=esphome)](https://github.com/andrewjswan/esphome-components/)
[![esphome_badge](https://img.shields.io/static/v1?label=ESPHome&message=Config&color=blue&logo=esphome)](https://esphome.io/)
[![GitHub](https://img.shields.io/github/license/andrewjswan/matrix-lamp?color=blue)](https://github.com/andrewjswan/matrix-lamp/blob/main/LICENSE)
[![GitHub release (latest SemVer including pre-releases)](https://img.shields.io/github/v/release/andrewjswan/matrix-lamp?include_prereleases)](https://github.com/andrewjswan/matrix-lamp/releases)
[![StandWithUkraine](https://raw.githubusercontent.com/vshymanskyy/StandWithUkraine/main/badges/StandWithUkraine.svg)](https://github.com/vshymanskyy/StandWithUkraine/blob/main/docs/README.md)

## Description
A compilation of firmware adaptations based on the original idea of Gyver Lamp, for ESPHome.

## Firmware
You can install the firmware using [ESPHome](https://esphome.io/) using one of the ready-made configuration files or use [**MatrixLamp - ESP Web Tools**](https://andrewjswan.github.io/matrix-lamp/) - a convenient tool for installing and updating the firmware of ESP32 devices in the browser.

## Hardware
|       |ESP32  |ESP32C3 |ESP32S2 | ESP32S3 | 
|-------|-------|--------|--------|---------|
|LED    |GPIO3  |GPIO3   |GPIO3   |GPIO3    |
|Button |GPIO35 |GPIO10  |GPIO35  |GPIO35   |

### Settings override

If you need to change any settings, you can override them using `substitutions`, for example:
```Yaml
substitutions:
  pin_pullup: "false"
  pin_leds: "GPIO32"
  color_correct_bright: "70%"
```

### Matrix Lamp Component
```Yaml
external_components:
  - source:
      type: git
      url: https://github.com/andrewjswan/matrix-lamp
      ref: main
    components: [matrix_lamp]

matrix_lamp:
  id: matrix
  width: 16                       # Matrix width
  height: 16                      # Matrix height
  random: true                    # The effects will be turned on at random (but successful) Speed ​nd Scale settings
  intensity_id: matrix_intensity  # Template number identifier for intensity control
  scale_id: matrix_scale          # Template number identifier for scale control
  speed_id: matrix_speed          # Template number identifier for speed control
  matrix_orientation: 5           # Matrix orientation [0..7] If not specified, it will be possible to change it on the fly using the SetMatrixOrientation function
  matrix_type: 0                  # Matrix type [0..1] If not specified, it will be possible to change it on the fly using the SetMatrixType function


light:
  - platform: ...
    effects:
...
      - matrix_lamp_effect:
          name: Peacock
          mode: RAINBOW_STRIPE

      - matrix_lamp_effect:
          name: Fire 2021
          mode: FIRE_2021
...
```

#### Parameters reference

**id** (optional, ID): Manually specify the identifier used for code generation and in service definitions.

**width** (optional, int): Matrix width

**height** (optional, int):  Matrix height

**random** (optional, boolean): The effects will be turned on at random (but successful) Speed ​nd Scale settings

**intensity_id** (optional, ID): Template number identifier for intensity control

**scale_id** (optional, ID): Template number identifier for scale control

**speed_id** (optional, ID): Template number identifier for speed control

**display** (optional, ID): Display component identifier for Icons show

**matrix_orientation** (optional, int): Matrix orientation [0..7] If not specified, it will be possible to change it on the fly using the SetMatrixOrientation function

**matrix_type** (optional, int): Matrix type [0..1] If not specified, it will be possible to change it on the fly using the SetMatrixType function

**iconscache** (optional, boolean): If true, it caches icons in the `.cache\icons` folder and if it finds the specified icons in the cache, it uses them instead of trying to download them again from the Internet. (default = `false`)

**bitmap** (optional, boolean): if true, adds a service `show_bitmap` for displaying icons defined as an rgb565 array.

**settings_function** (optional, boolean): if true, adds a services `set_effect_intensity`, `set_effect_speed`, `set_effect_scale`, `reset_effect_settings`.


### Icons and Animations

Download and install all needed icons (.jpg/.png) and animations (.GIF) under the `matrix_lamp:` key. All icons have to be 8x8 pixels in size. If necessary, you can scale them with the option `resize: 8x8`.

You can also specify a URL to directly download the image file. The URLs will only be downloaded once at compile time, so there is no additional traffic on the hosting website.

All other solutions provide ready-made icons, especially **Lametric** has a big database of [icons](https://developer.lametric.com/icons). If you find an icon, you can use it with its ID. e.g., `lameid: 1234` Please, check the copyright of the icons you use. The maximum number of icons is limited to 100 in the code and also by the flash space and the RAM of your board.

```Yaml
matrix_lamp:
  display: ...
  icons: 
    - id: boot 
      file: icons/rocket.GIF
      duration: 75     
    - id: temp 
      lameid: 1234
    - id: yoga
      file: icons/yoga-bridge.GIF
      pingpong: true
    - id: jackshome
      url: https://awtrix.blueforcer.de/animations/JackHomePage
      resize: 32x8
    - id: garage
      lameid: 1234
      duration: 100
    - id: homeassistant
      url: https://github.com/home-assistant/assets/raw/master/logo/logo-special.png      
```

The ID of the icons is used later to icon display. So, you should name them wisely. If you like to group icons, you should prefix them, e.g., with “weather_”.

There are some more advanced parameters:

- **frame_duration** (optional, ms): in the case of a GIF-file, the component tries to read the default interval for each frame. The default/fallback interval is 192 ms. In case you need to override the default value, set the duration per icon.
- **pingpong** (optional, boolean): in the case of a GIF-file, you can reverse the frames instead of starting from the first frame.

- **file** (Exclusive, filename): a local filename
- **url** (Exclusive, url): a URL to download the icon
- **lameid** (Exclusive, number): the ID from the LaMetric icon database

- **resize** (Optional, "8x8" or "32x8"): If the source file hasn't the right dimensions.

```Yaml
    - id: jackshome
      url: https://awtrix.blueforcer.de/animations/JackHomePage
      resize: 32x8
```

### API 

#### Home Assistant services

There are services and parameters to use.

You can call this from, e.g., the developer tools service.

[![Open your Home Assistant instance with the developer tools service.](https://my.home-assistant.io/badges/developer_services.svg)](https://my.home-assistant.io/redirect/developer_services/)

```c
  brightness {"value"}
  show_icon {"icon_name"}
  show_bitmap {"bitmap"}
  hide_icon

  set_effect_intensity {"value"}
  set_effect_speed {"value"}
  set_effect_scale {"value"}
  reset_effect_settings
```

#### Lambda

You can use the above functions also in [lambdas](https://esphome.io/automations/actions#lambda-action) in your esphome YAML.

```c
  void set_brightness(int value);
  void show_icon(std::string icon);
  void show_icon_by_index(int icon);
  void show_bitmap(std::string bitmap);
  void hide_icon();

  // Reset the current effect, for example when changing the lamp state. 
  void ResetCurrentEffect();

#ifndef ORIENTATION
  // Set Matrix Orientation [1..8] instead of real matrix ORIENTATION [0..7]
  bool SetMatrixOrientation(uint8_t orientation);
#endif

#ifndef MATRIX_TYPE
  // Set Matrix Type [1..2] instead of real matrix MATRIX_TYPE [0..1]
  bool SetMatrixType(uint8_t type);
#endif

  // Set intensity for effect
  void SetIntensityForEffect(uint8_t mode, uint8_t intensity);
  // Set scale for effect
  void SetScaleForEffect(uint8_t mode, uint8_t scale);
  // Set speed for effect
  void SetSpeedForEffect(uint8_t mode, uint8_t speed);
  // Set scale from Color for effect
  void SetScaleFromColorForEffect(uint8_t mode, Color color);
  // Set custom effect
  void SetCustomEffect(uint8_t mode);

#if defined(RANDOM_SETTINGS)
  bool GetRandomSettings();
  void SetRandomSettings(bool b=false);
#endif
```

<br />

> [!TIP]
> Various examples of additional functionality of Matrix Lamp, [Packages](https://github.com/andrewjswan/matrix-lamp/tree/main/packages), [Examples](https://github.com/andrewjswan/matrix-lamp/tree/main/examples)

> [!TIP]
> ESPHome [configuration](https://github.com/andrewjswan/esphome-config)
