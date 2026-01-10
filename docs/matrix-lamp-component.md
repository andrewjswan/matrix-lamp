## Matrix Lamp Component

!!! example annotate "Matrix Lamp Component"

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
      width: 16 (1)
      height: 16 (2)
      random: true (3)
      intensity_id: matrix_intensity (4)
      scale_id: matrix_scale (5)
      speed_id: matrix_speed (6)
      matrix_orientation: 5 (7)
      matrix_type: 0 (8)


    light:
      - platform: ...
        effects:
    ...
          - matrix_lamp_effect:
              name: Peacock
              mode: RAINBOW_STRIPE (9)

          - matrix_lamp_effect:
              name: Fire 2021
              mode: FIRE_2021 (10)
    ...
    ```
1. Matrix width
2. Matrix height
3. The effects will be turned on at random (but successful) `Speed` and `Scale` settings
4. Template number identifier for `intensity` control
5. Template number identifier for `scale` control
6. Template number identifier for `speed` control
7. Matrix orientation `[0..7]` If not specified, it will be possible to change it on the fly using the `SetMatrixOrientation` function
8. Matrix type `[0..1]` If not specified, it will be possible to change it on the fly using the `SetMatrixType` function
9.  See the list of all possible [modes](effects.md) and their values
10.  See the list of all possible [modes](effects.md) and their values

### Parameters reference

- **id** (optional, ID): Manually specify the identifier used for code generation and in service definitions.

- **width** (optional, int): Matrix width

- **height** (optional, int):  Matrix height

- **random** (optional, boolean): The effects will be turned on at random (but successful) `Speed` and `Scale` settings

- **intensity_id** (optional, ID): Template number identifier for `intensity`[^1] control

- **scale_id** (optional, ID): Template number identifier for `scale`[^1] control

- **speed_id** (optional, ID): Template number identifier for `speed`[^1] control

- **display** (optional, ID): Display component identifier for Icons show

- **matrix_orientation** (optional, int): Matrix orientation `[0..7]` If not specified, it will be possible to change it on the fly using the `SetMatrixOrientation` function

- **matrix_type** (optional, int): Matrix type `[0..1]` If not specified, it will be possible to change it on the fly using the `SetMatrixType` function

- **iconscache** (optional, boolean): If true, it caches icons in the `.cache\icons` folder and if it finds the specified icons in the cache, it uses them instead of trying to download them again from the Internet. (default = `false`)

- **bitmap** (optional, boolean): if true, adds a service `show_bitmap` for displaying icons defined as an rgb565 array.

[^1]: Not all effects support changing this parameter.

!!! example annotate "Matrix Orientation"

    ``` { .yaml .copy .annotate }
    number:
      - platform: template
        name: Matrix Orientation
        icon: mdi:autorenew
        min_value: 1 (1)
        max_value: 8 (2)
        step: 1
        optimistic: true
        id: matrix_orientation
        initial_value: 4
        mode: slider
        entity_category: config
        disabled_by_default: true
        restore_value: true
        on_value:
          then:
            - lambda: |-
                id(matrix).set_matrix_orientation(x); (3)
    ```

1. The `set_matrix_orientation` function accepts orientation values in the range `[1..8]`, although the component itself uses `[0..7]`.
2. The `set_matrix_orientation` function accepts orientation values in the range `[1..8]`, although the component itself uses `[0..7]`.
3. The `set_matrix_orientation` function accepts orientation values in the range `[1..8]`, although the component itself uses `[0..7]`.

## Custom Services

!!! note "Custom Services"
    The `set_effect_intensity`, `set_effect_speed`, `set_effect_scale`, `reset_effect_settings` services in Home Assistant will be available when the [`custom_services`](https://esphome.io/components/api/) option is enabled, which enables the compilation of custom API services for external components that use the C++ `CustomAPIDevice` class.

## Local triggers

#### on_effect_start

This trigger is called whenever one of the component's Matrix Lamp effects is turned on. In lambda's you can use local variables:

- **effect** (effect number, uint8_t): Effect number in Matrix Lamp component.

- **intensity** (intensity, uint8_t): Intensity of effect.

- **speed** (speed uint8_t): Speed of effect.

- **scale** (scale uint8_t): Scale of effect.

!!! example annotate "On effect start trigger"

    ``` { .yaml .copy .annotate }
    matrix_lamp:
      id: matrix
      on_effect_start:
        - logger.log:
            format: "${friendly_name} Effect start, %d %d %d %d"
            args:
              - effect
              - intensity
              - speed
              - scale
        - espnow.broadcast:
            data: !lambda "return {effect, intensity, speed, scale};"
            wait_for_sent: true
            on_sent:
              - logger.log: "ESPNow broadcast message sent successfully"
            on_error:
              - logger.log: "ESPNow broadcast  message failed to send"
            continue_on_error: true
    ```
