## Matrix Lamp Component

``` { .yaml .copy .annotate }
external_components:
  - source:
      type: git
      url: https://github.com/andrewjswan/matrix-lamp
      ref: main
    components: [fastled_helper, matrix_lamp]

fastled_helper:
  palettes: false

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
          mode: RAINBOW_STRIPE (1)

      - matrix_lamp_effect:
          name: Fire 2021
          mode: FIRE_2021 (1)
...
```

1.  See the list of all possible [modes](effects.md) and their values

## Parameters reference

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

